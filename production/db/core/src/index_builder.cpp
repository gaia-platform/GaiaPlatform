/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/db/index_builder.hpp"

#include <iostream>
#include <utility>

#include "gaia/exceptions.hpp"

#include "gaia_internal/db/catalog_core.hpp"

#include "data_holder.hpp"
#include "db_helpers.hpp"
#include "field_access.hpp"
#include "hash_index.hpp"
#include "range_index.hpp"
#include "reflection.hpp"
#include "txn_metadata.hpp"
#include "type_id_mapping.hpp"

using namespace gaia::common;

namespace gaia
{
namespace db
{
namespace index
{

/*
w - the writer guard for the index meant to be truncated.
commit_ts - the cutoff timestamp below which we remove committed transactions (inclusive).
*/
template <class T_index>
void truncate_index(index_writer_guard_t<T_index>& w, gaia_txn_id_t commit_ts)
{
    auto index = w.get_index();
    auto it_next = index.begin();
    auto it_end = index.end();

    while (it_next != it_end)
    {
        auto it_current = it_next++;
        auto ts = transactions::txn_metadata_t::get_commit_ts_from_begin_ts(it_current->second.txn_id);

        // Ignore invalid txn_ids, txn could be in-flight at the point of testing.
        if (ts <= commit_ts && ts != c_invalid_gaia_txn_id)
        {
            // Only erase entry if each key contains at least one additional entry with the same key.
            if (it_next != it_end && it_current->first == it_next->first)
            {
                it_next = index.erase(it_current);
            }
        }
    }
}

index_key_t index_builder_t::make_key(gaia_id_t index_id, gaia_type_t type_id, const uint8_t* payload)
{
    ASSERT_PRECONDITION(payload, "Cannot compute key on null payloads.");

    index_key_t index_key;
    gaia_id_t type_record_id = type_id_mapping_t::instance().get_record_id(type_id);

    ASSERT_INVARIANT(
        type_record_id != c_invalid_gaia_id,
        "The type '" + std::to_string(type_id) + "' does not exist in the catalog.");

    auto table = catalog_core_t::get_table(type_record_id);
    auto schema = table.binary_schema();
    auto index_view = index_view_t(id_to_ptr(index_id));

    const auto& fields = *(index_view.fields());
    for (gaia_id_t field_id : fields)
    {
        field_position_t pos = field_view_t(id_to_ptr(field_id)).position();
        index_key.insert(payload_types::get_field_value(type_id, payload, schema->data(), schema->size(), pos));
    }

    return index_key;
}

void index_builder_t::serialize_key(const index_key_t& key, data_write_buffer_t& buffer)
{
    for (const payload_types::data_holder_t& key_value : key.values())
    {
        key_value.serialize(buffer);
    }
}

index_key_t index_builder_t::deserialize_key(common::gaia_id_t index_id, data_read_buffer_t& buffer)
{
    ASSERT_PRECONDITION(index_id != c_invalid_gaia_id, "Invalid gaia id.");

    index_key_t index_key;
    auto index_ptr = id_to_ptr(index_id);

    ASSERT_INVARIANT(index_ptr != nullptr, "Cannot find catalog entry for index.");

    auto index_view = index_view_t(index_ptr);

    const auto& fields = *(index_view.fields());
    for (auto field_id : fields)
    {
        data_type_t type = field_view_t(id_to_ptr(field_id)).data_type();
        index_key.insert(payload_types::data_holder_t(buffer, convert_to_reflection_type(type)));
    }

    return index_key;
}

index_record_t index_builder_t::make_record(
    gaia::db::gaia_locator_t locator,
    gaia::db::gaia_offset_t offset,
    index_record_operation_t operation)
{
    ASSERT_PRECONDITION(
        operation != index_record_operation_t::not_set,
        "A valid operation should be set in each index record!");

    return index_record_t{get_current_txn_id(), locator, offset, operation};
}

bool index_builder_t::index_exists(common::gaia_id_t index_id)
{
    return get_indexes()->find(index_id) != get_indexes()->end();
}

indexes_t::iterator index_builder_t::create_empty_index(const index_view_t& index_view)
{
    bool is_unique = index_view.unique();

    switch (index_view.type())
    {
    case catalog::index_type_t::range:
        return get_indexes()->emplace(
                                index_view.id(),
                                std::make_shared<range_index_t, gaia_id_t, bool>(
                                    std::forward<gaia_id_t>(index_view.id()), std::forward<bool>(is_unique)))
            .first;
        break;

    case catalog::index_type_t::hash:
        return get_indexes()->emplace(
                                index_view.id(),
                                std::make_shared<hash_index_t, gaia_id_t, bool>(
                                    std::forward<gaia_id_t>(index_view.id()), std::forward<bool>(is_unique)))
            .first;
        break;
    }
}

template <typename T_index_type>
void update_index_entry(
    base_index_t* base_index, bool is_unique, index_key_t&& key, index_record_t record)
{
    ASSERT_PRECONDITION(
        record.operation != index_record_operation_t::not_set,
        "A valid operation was expected in the index record!");

    auto index = static_cast<T_index_type*>(base_index);

    // If the index has UNIQUE constraint, then we need to prevent inserting duplicate values.
    // We need this check only for insertions.
    // Our checks also require access to txn_metadata_t, so they can only be performed on the server.
    if (is_unique
        && record.operation == index_record_operation_t::insert
        && transactions::txn_metadata_t::is_txn_metadata_map_initialized())
    {
        // BULK lock to avoid race condition where two different txns can insert the same value.
        auto index_guard = index->get_writer();

        auto search_result = index_guard.equal_range(key);
        auto& it_start = search_result.first;
        auto& it_end = search_result.second;

        bool has_found_duplicate_key = false;

        while (it_start != it_end)
        {
            gaia_txn_id_t begin_ts = it_start->second.txn_id;
            gaia_txn_id_t commit_ts
                = transactions::txn_metadata_t::get_commit_ts_from_begin_ts(begin_ts);

            ASSERT_PRECONDITION(
                transactions::txn_metadata_t::is_begin_ts(begin_ts),
                "Transaction id in index key entry is not a begin timestamp!");

            // Index entries made by rolled back transactions or aborted transactions can be ignored,
            // We can also remove them, because we are already holding a lock.
            bool is_aborted_operation = (commit_ts != c_invalid_gaia_txn_id && transactions::txn_metadata_t::is_txn_aborted(commit_ts));
            if (transactions::txn_metadata_t::is_txn_terminated(begin_ts)
                || is_aborted_operation)
            {
                it_start = index_guard.get_index().erase(it_start);
                continue;
            }

            // The list we iterate over reflects the order of operations.
            // Updates that don't change the index key are harmless
            // and those that do change it are indicated as separate removals and insertions.
            // Hence, we only care about insertions and removals;
            // The key exists if the last seen operation is not a committed removal
            // or our own transaction's removal.
            // We ignore uncommitted removals by other transactions,
            // because if their transaction is aborted,
            // it would allow us to perform a duplicate insertion.
            bool is_our_operation = (begin_ts == record.txn_id);
            bool is_committed_operation = (commit_ts != c_invalid_gaia_txn_id && transactions::txn_metadata_t::is_txn_committed(commit_ts));
            if (it_start->second.operation == index_record_operation_t::remove
                && (is_our_operation || is_committed_operation))
            {
                has_found_duplicate_key = false;
            }
            else if (it_start->second.operation == index_record_operation_t::insert)
            {
                has_found_duplicate_key = true;
            }

            ++it_start;
        }

        if (has_found_duplicate_key)
        {
            auto index_view = index_view_t(id_to_ptr(index->id()));
            auto table_view = table_view_t(id_to_ptr(index_view.table_id()));
            throw unique_constraint_violation(index_view.name(), table_view.name());
        }

        index->insert_index_entry(std::move(key), record);
    }
    else
    {
        index->insert_index_entry(std::move(key), record);
    }
}

void index_builder_t::update_index(gaia_id_t index_id, index_key_t&& key, index_record_t record, bool allow_create_empty)
{
    ASSERT_PRECONDITION(get_indexes(), "Indexes are not initialized.");

    auto it = get_indexes()->find(index_id);

    if (allow_create_empty && it == get_indexes()->end())
    {
        auto index_ptr = id_to_ptr(index_id);
        ASSERT_INVARIANT(index_ptr != nullptr, "Cannot find index in catalog.");
        auto index_view = index_view_t(index_ptr);
        it = index::index_builder_t::create_empty_index(index_view);
    }
    else
    {
        ASSERT_INVARIANT(it != get_indexes()->end(), "Index structure could not be found.");
    }

    bool is_unique_index = it->second->is_unique();

    switch (it->second->type())
    {
    case catalog::index_type_t::range:
        update_index_entry<range_index_t>(it->second.get(), is_unique_index, std::move(key), record);
        break;
    case catalog::index_type_t::hash:
        update_index_entry<hash_index_t>(it->second.get(), is_unique_index, std::move(key), record);
        break;
    }
}

void index_builder_t::update_index(
    gaia::common::gaia_id_t index_id, gaia_type_t type_id, const txn_log_t::log_record_t& log_record, bool allow_create_empty)
{
    // Most operations expect an object located at new_offset,
    // so we'll try to get a reference to its payload.
    auto obj = offset_to_ptr(log_record.new_offset);
    auto payload = (obj) ? reinterpret_cast<const uint8_t*>(obj->data()) : nullptr;

    switch (log_record.operation)
    {
    case gaia_operation_t::create:
        index_builder_t::update_index(
            index_id,
            index_builder_t::make_key(index_id, type_id, payload),
            index_builder_t::make_record(
                log_record.locator, log_record.new_offset, index_record_operation_t::insert),
            allow_create_empty);
        break;
    case gaia_operation_t::update:
    {
        auto old_obj = offset_to_ptr(log_record.old_offset);
        auto old_payload = (old_obj) ? reinterpret_cast<const uint8_t*>(old_obj->data()) : nullptr;
        index_key_t old_key = index_builder_t::make_key(index_id, type_id, old_payload);
        index_key_t new_key = index_builder_t::make_key(index_id, type_id, payload);

        // If the index key is not changed, mark operation as an update.
        // Otherwise, we'll mark it as two individual remove/insert operations,
        // because it's semantically indistinguishable from our transaction just doing that.
        if (new_key == old_key)
        {
            index_builder_t::update_index(
                index_id,
                std::move(new_key),
                index_builder_t::make_record(
                    log_record.locator, log_record.new_offset, index_record_operation_t::update),
                allow_create_empty);
        }
        else
        {
            index_builder_t::update_index(
                index_id,
                std::move(old_key),
                index_builder_t::make_record(
                    log_record.locator, log_record.old_offset, index_record_operation_t::remove),
                allow_create_empty);
            index_builder_t::update_index(
                index_id,
                std::move(new_key),
                index_builder_t::make_record(
                    log_record.locator, log_record.new_offset, index_record_operation_t::insert),
                allow_create_empty);
        }
    }
    break;
    case gaia_operation_t::remove:
    {
        auto old_obj = offset_to_ptr(log_record.old_offset);
        auto old_payload = (old_obj) ? reinterpret_cast<const uint8_t*>(old_obj->data()) : nullptr;
        index_builder_t::update_index(
            index_id,
            index_builder_t::make_key(index_id, type_id, old_payload),
            index_builder_t::make_record(
                log_record.locator, log_record.old_offset, index_record_operation_t::remove),
            allow_create_empty);
    }
    break;
    default:
        ASSERT_UNREACHABLE("Cannot handle log type");
        return;
    }
}

void index_builder_t::populate_index(common::gaia_id_t index_id, common::gaia_type_t type_id, gaia_locator_t locator)
{
    auto payload = reinterpret_cast<const uint8_t*>(locator_to_ptr(locator)->data());
    update_index(
        index_id,
        make_key(index_id, type_id, payload),
        make_record(locator, locator_to_offset(locator), index_record_operation_t::insert));
}

void index_builder_t::truncate_index_to_ts(common::gaia_id_t index_id, gaia_txn_id_t commit_ts)
{
    ASSERT_PRECONDITION(get_indexes(), "Indexes not initialized");
    auto it = get_indexes()->find(index_id);

    if (it == get_indexes()->end())
    {
        throw index_not_found(index_id);
    }

    switch (it->second->type())
    {
    case catalog::index_type_t::range:
    {
        auto index = static_cast<range_index_t*>(it->second.get());
        auto w = index->get_writer();
        truncate_index(w, commit_ts);
    }
    break;
    case catalog::index_type_t::hash:
    {
        auto index = static_cast<hash_index_t*>(it->second.get());
        auto w = index->get_writer();
        truncate_index(w, commit_ts);
    }
    break;
    }
}

void index_builder_t::update_indexes_from_logs(
    const txn_log_t& records, bool skip_catalog_integrity_check, bool allow_create_empty)
{
    for (size_t i = 0; i < records.record_count; ++i)
    {
        auto& log_record = records.log_records[i];
        db_object_t* obj = nullptr;

        if (log_record.operation == gaia_operation_t::remove)
        {
            obj = offset_to_ptr(log_record.old_offset);
            ASSERT_INVARIANT(obj != nullptr, "Cannot find db object.");
        }
        else
        {
            obj = offset_to_ptr(log_record.new_offset);
            ASSERT_INVARIANT(obj != nullptr, "Cannot find db object.");
        }

        // Flag the type_id_mapping cache to clear if any changes in the schema are detected.
        if (obj->type == static_cast<gaia_type_t>(system_table_type_t::catalog_gaia_table))
        {
            type_id_mapping_t::instance().clear();
            break;
        }
    }

    for (size_t i = 0; i < records.record_count; ++i)
    {
        auto& log_record = records.log_records[i];
        db_object_t* obj = nullptr;

        if (log_record.operation == gaia_operation_t::remove)
        {
            obj = offset_to_ptr(log_record.old_offset);
            ASSERT_INVARIANT(obj != nullptr, "Cannot find db object.");
        }
        else
        {
            obj = offset_to_ptr(log_record.new_offset);
            ASSERT_INVARIANT(obj != nullptr, "Cannot find db object.");
        }

        gaia_id_t type_record_id = type_id_mapping_t::instance().get_record_id(obj->type);

        // New index object.
        if (obj->type == static_cast<gaia_type_t>(catalog_table_type_t::gaia_index))
        {
            auto index_view = index_view_t(obj);
            index::index_builder_t::create_empty_index(index_view);
        }

        // System tables are not indexed.
        // Skip if catalog verification disabled and type not found in the catalog.
        if (is_system_object(obj->type) || (skip_catalog_integrity_check && type_record_id == c_invalid_gaia_id))
        {
            continue;
        }

        for (const auto& index : catalog_core_t::list_indexes(type_record_id))
        {
            index::index_builder_t::update_index(index.id(), obj->type, log_record, allow_create_empty);
        }
    }
}

} // namespace index
} // namespace db
} // namespace gaia
