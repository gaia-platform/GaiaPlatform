/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/db/index_builder.hpp"

#include <array>
#include <utility>
#include <vector>

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

index_key_t index_builder_t::make_key(db_index_t index, const uint8_t* payload)
{
    ASSERT_PRECONDITION(payload, "Cannot compute key on null payloads.");

    return index_key_t(index->key_schema(), payload);
}

void index_builder_t::serialize_key(const index_key_t& key, payload_types::data_write_buffer_t& buffer)
{
    for (const payload_types::data_holder_t& key_value : key.values())
    {
        // TODO: This will have to do until catalog information is available!
        bool optional = true;
        key_value.serialize(buffer, optional);
    }
}

index_key_t index_builder_t::deserialize_key(common::gaia_id_t index_id, payload_types::data_read_buffer_t& buffer)
{
    ASSERT_PRECONDITION(index_id.is_valid(), "Invalid gaia id.");

    index_key_t index_key;
    auto index_ptr = id_to_ptr(index_id);

    ASSERT_INVARIANT(index_ptr != nullptr, "Cannot find catalog entry for index.");

    auto index_view = catalog_core::index_view_t(index_ptr);

    const auto& fields = *(index_view.fields());
    for (auto field_id : fields)
    {
        data_type_t type = catalog_core::field_view_t(id_to_ptr(field_id)).data_type();
        // TODO: Until this information is available in the catalog, this will have to do!
        bool optional = true;
        index_key.insert(payload_types::data_holder_t(buffer, convert_to_reflection_type(type), optional));
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

    return index_record_t{get_current_txn_id(), locator, offset, operation, 0};
}

bool index_builder_t::index_exists(common::gaia_id_t index_id)
{
    return get_indexes()->find(index_id) != get_indexes()->end();
}

indexes_t::iterator index_builder_t::create_empty_index(const catalog_core::index_view_t& index_view, bool skip_catalog_integrity_check)
{
    ASSERT_PRECONDITION(skip_catalog_integrity_check || index_view.table_id() != c_invalid_gaia_id, "Cannot find table for index.");

    bool is_unique = index_view.unique();

    index_key_schema_t key_schema;

    if (index_view.table_id().is_valid())
    {
        auto table_view = catalog_core::table_view_t(id_to_ptr(index_view.table_id()));
        key_schema.table_type = table_view.table_type();
        key_schema.binary_schema = table_view.binary_schema();

        const auto& fields = *(index_view.fields());
        for (gaia_id_t field_id : fields)
        {
            field_position_t pos = catalog_core::field_view_t(id_to_ptr(field_id)).position();
            key_schema.field_positions.push_back(pos);
        }
    }

    switch (index_view.type())
    {
    case catalog::index_type_t::range:
        return get_indexes()->emplace(
                                index_view.id(),
                                std::make_shared<range_index_t, gaia_id_t, index_key_schema_t, bool>(
                                    std::forward<gaia_id_t>(index_view.id()),
                                    std::forward<index_key_schema_t>(key_schema),
                                    std::forward<bool>(is_unique)))
            .first;
        break;

    case catalog::index_type_t::hash:
        return get_indexes()->emplace(
                                index_view.id(),
                                std::make_shared<hash_index_t, gaia_id_t, index_key_schema_t, bool>(
                                    std::forward<gaia_id_t>(index_view.id()),
                                    std::forward<index_key_schema_t>(key_schema),
                                    std::forward<bool>(is_unique)))
            .first;
        break;
    }
}

void index_builder_t::drop_index(const catalog_core::index_view_t& index_view)
{
    size_t dropped = get_indexes()->erase(index_view.id());
    // We expect 0 or 1 index structures to be dropped here.
    // It is possible for this value to be 0 if a created index is dropped, but it wasn't
    // lazily created.
    ASSERT_INVARIANT(dropped <= 1, "Unexpected number of index structures removed!");
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
    //
    // We also skip the checks for NULL keys.
    if (is_unique
        && record.operation == index_record_operation_t::insert
        && !key.is_null()
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
                = (is_marked_committed(it_start->second))
                ? c_invalid_gaia_txn_id
                : transactions::txn_metadata_t::get_commit_ts_from_begin_ts(begin_ts);

            ASSERT_PRECONDITION(
                is_marked_committed(it_start->second) || transactions::txn_metadata_t::is_begin_ts(begin_ts),
                "Transaction id in index key entry is not marked committed or a begin timestamp!");

            bool is_aborted_operation
                = !is_marked_committed(it_start->second)
                && commit_ts.is_valid()
                && transactions::txn_metadata_t::is_txn_aborted(commit_ts);

            // Index entries made by rolled back transactions or aborted transactions can be ignored.
            // We can also remove them, because we are already holding a lock.

            if (is_aborted_operation
                || (!is_marked_committed(it_start->second)
                    && transactions::txn_metadata_t::is_txn_terminated(begin_ts)))
            {
                it_start = index_guard.get_index().erase(it_start);
                continue;
            }

            // The list we iterate over reflects the order of operations.
            // The key exists if the last seen operation is not a committed removal
            // or our own transaction's removal.
            // We ignore uncommitted removals by other transactions,
            // because if their transaction is aborted,
            // it would allow us to perform a duplicate insertion.
            bool is_our_operation = (begin_ts == record.txn_id);
            bool is_committed_operation
                = is_marked_committed(it_start->second)
                || (commit_ts.is_valid() && transactions::txn_metadata_t::is_txn_committed(commit_ts));

            // Opportunistically mark a record as committed to skip metadata lookup next time.
            if (is_committed_operation)
            {
                mark_committed(it_start->second);
            }

            if (it_start->second.operation == index_record_operation_t::remove)
            {
                // We ignore uncommitted removals by other transactions,
                // because if their transaction is aborted,
                // it would allow us to perform a duplicate insertion.
                if (is_our_operation || is_committed_operation)
                {
                    has_found_duplicate_key = false;
                }
            }
            else
            {
                // Flag inserts and updates with the same key.
                // Updates should be accounted for because the original inserts may have been garbage collected.
                has_found_duplicate_key = true;
            }

            ++it_start;
        }

        if (has_found_duplicate_key)
        {
            auto index_view = catalog_core::index_view_t(id_to_ptr(index->id()));
            auto table_view = catalog_core::table_view_t(id_to_ptr(index_view.table_id()));
            throw unique_constraint_violation_internal(table_view.name(), index_view.name());
        }

        index->insert_index_entry(std::move(key), record);
    }
    else
    {
        index->insert_index_entry(std::move(key), record);
    }
}

void index_builder_t::update_index(
    db_index_t index, index_key_t&& key, index_record_t record)
{
    bool is_unique_index = index->is_unique();

    switch (index->type())
    {
    case catalog::index_type_t::range:
        update_index_entry<range_index_t>(index.get(), is_unique_index, std::move(key), record);
        break;
    case catalog::index_type_t::hash:
        update_index_entry<hash_index_t>(index.get(), is_unique_index, std::move(key), record);
        break;
    }
}

void index_builder_t::update_index(
    db_index_t index,
    const log_record_t& log_record)
{
    // Most operations expect an object located at new_offset,
    // so we'll try to get a reference to its payload.
    auto obj = offset_to_ptr(log_record.new_offset);
    auto payload = (obj) ? reinterpret_cast<const uint8_t*>(obj->data()) : nullptr;

    switch (log_record.operation())
    {
    case gaia_operation_t::create:
        index_builder_t::update_index(
            index,
            index_builder_t::make_key(index, payload),
            index_builder_t::make_record(
                log_record.locator, log_record.new_offset, index_record_operation_t::insert));
        break;
    case gaia_operation_t::update:
    {
        auto old_obj = offset_to_ptr(log_record.old_offset);
        auto old_payload = (old_obj) ? reinterpret_cast<const uint8_t*>(old_obj->data()) : nullptr;
        index_key_t old_key = index_builder_t::make_key(index, old_payload);
        index_key_t new_key = index_builder_t::make_key(index, payload);

        // If the index key is not changed, mark operation as an update.
        // Otherwise, we'll mark it as two individual remove/insert operations,
        // because it's semantically indistinguishable from our transaction just doing that.
        if (new_key == old_key)
        {
            index_builder_t::update_index(
                index,
                std::move(new_key),
                index_builder_t::make_record(
                    log_record.locator, log_record.new_offset, index_record_operation_t::update));
        }
        else
        {
            index_builder_t::update_index(
                index,
                std::move(old_key),
                index_builder_t::make_record(
                    log_record.locator, log_record.old_offset, index_record_operation_t::remove));
            index_builder_t::update_index(
                index,
                std::move(new_key),
                index_builder_t::make_record(
                    log_record.locator, log_record.new_offset, index_record_operation_t::insert));
        }
    }
    break;
    case gaia_operation_t::remove:
    {
        auto old_obj = offset_to_ptr(log_record.old_offset);
        auto old_payload = (old_obj) ? reinterpret_cast<const uint8_t*>(old_obj->data()) : nullptr;
        index_builder_t::update_index(
            index,
            index_builder_t::make_key(index, old_payload),
            index_builder_t::make_record(
                log_record.locator, log_record.old_offset, index_record_operation_t::remove));
    }
    break;
    default:
        ASSERT_UNREACHABLE("Cannot handle log type");
        return;
    }
}

void index_builder_t::populate_index(common::gaia_id_t index_id, gaia_locator_t locator)
{
    ASSERT_PRECONDITION(get_indexes(), "Indexes are not initialized.");
    auto payload = reinterpret_cast<const uint8_t*>(locator_to_ptr(locator)->data());

    auto it = get_indexes()->find(index_id);
    ASSERT_INVARIANT(it != get_indexes()->end(), "Index structure could not be found.");
    db_index_t index = it->second;

    update_index(
        index,
        make_key(index, payload),
        make_record(locator, locator_to_offset(locator), index_record_operation_t::insert));
}

/*
 * This method performs index maintenance operations based on logs.
 * The order of operations in the index data structure is based on the same ordering as the logs.
 * As such, we rely on the logs being sorted by temporal order.
 */
void index_builder_t::update_indexes_from_txn_log(
    txn_log_t* txn_log, bool skip_catalog_integrity_check, bool allow_create_empty)
{
    // Clear the type_id_mapping cache (so it will be refreshed) if we find any
    // table is created or dropped in the txn.
    // Keep track of dropped tables.
    bool has_cleared_cache = false;
    std::vector<gaia_type_t> dropped_types;

    for (auto log_record = txn_log->log_records; log_record < txn_log->log_records + txn_log->record_count; ++log_record)
    {
        if ((log_record->operation() == gaia_operation_t::remove || log_record->operation() == gaia_operation_t::create)
            && offset_to_ptr(
                   log_record->operation() == gaia_operation_t::remove
                       ? log_record->old_offset
                       : log_record->new_offset)
                    ->type
                == static_cast<gaia_type_t::value_type>(system_table_type_t::catalog_gaia_table))
        {
            if (!has_cleared_cache)
            {
                type_id_mapping_t::instance().clear();
                has_cleared_cache = true;
            }

            if (log_record->operation() == gaia_operation_t::remove)
            {
                auto table_view = catalog_core::table_view_t(offset_to_ptr(log_record->old_offset));
                dropped_types.push_back(table_view.table_type());
            }
        }
    }

    gaia_operation_t last_index_operation = gaia_operation_t::not_set;
    for (auto log_record = txn_log->log_records; log_record < txn_log->log_records + txn_log->record_count; ++log_record)
    {
        db_object_t* obj = nullptr;

        if (log_record->operation() == gaia_operation_t::remove)
        {
            obj = offset_to_ptr(log_record->old_offset);
            ASSERT_INVARIANT(obj != nullptr, "Cannot find db object.");
        }
        else
        {
            obj = offset_to_ptr(log_record->new_offset);
            ASSERT_INVARIANT(obj != nullptr, "Cannot find db object.");
        }

        // Maintenance on the in-memory index data structures.
        if (obj->type == static_cast<gaia_type_t::value_type>(catalog_core_table_type_t::gaia_index))
        {
            auto index_view = catalog_core::index_view_t(obj);

            if (log_record->operation() == gaia_operation_t::remove)
            {
                index::index_builder_t::drop_index(index_view);
            }
            // We only create the empty index after the post-create update operation because it is finally linked to the parent.
            else if (log_record->operation() == gaia_operation_t::update && last_index_operation == gaia_operation_t::create)
            {
                index::index_builder_t::create_empty_index(index_view, skip_catalog_integrity_check);
            }

            // Keep track of the last index operation in this txn.
            last_index_operation = log_record->operation();
            continue;
        }

        gaia_id_t type_record_id = type_id_mapping_t::instance().get_record_id(obj->type);

        // Catalog core tables are not indexed.
        // The operation is from a dropped table.
        // Skip if catalog verification disabled and type not found in the catalog.
        if (is_catalog_core_object(obj->type)
            || std::find(dropped_types.begin(), dropped_types.end(), obj->type) != dropped_types.end()
            || (skip_catalog_integrity_check && !type_record_id.is_valid()))
        {
            continue;
        }

        for (const auto& index : catalog_core::list_indexes(type_record_id))
        {
            ASSERT_PRECONDITION(get_indexes(), "Indexes are not initialized.");
            auto it = get_indexes()->find(index.id());

            if (allow_create_empty && it == get_indexes()->end())
            {
                auto index_ptr = id_to_ptr(index.id());
                ASSERT_INVARIANT(index_ptr != nullptr, "Cannot find index in catalog.");
                auto index_view = catalog_core::index_view_t(index_ptr);
                it = index::index_builder_t::create_empty_index(index_view);
            }
            else
            {
                ASSERT_INVARIANT(it != get_indexes()->end(), "Index structure could not be found.");
            }

            index::index_builder_t::update_index(it->second, *log_record);
        }
    }
}

template <class T_index>
void remove_entries_with_offsets(base_index_t* base_index, const index_offset_buffer_t& offsets, gaia_txn_id_t txn_id)
{
    auto index = static_cast<T_index*>(base_index);

    for (size_t i = 0; i < offsets.size(); ++i)
    {
        if (offsets.get_type(i) == index->table_type())
        {
            gaia_offset_t offset = offsets.get_offset(i);
            auto obj = offset_to_ptr(offset);
            index_key_t key = index_key_t(index->key_schema(), reinterpret_cast<const uint8_t*>(obj->data()));

            index->remove_index_entry_with_offset(key, offset, txn_id);
        }
    }
}

void index_builder_t::gc_indexes_from_txn_log(txn_log_t* txn_log, bool deallocate_new_offsets)
{
    size_t record_index = 0;
    size_t record_count = txn_log->record_count;

    while (record_index < record_count)
    {
        index_offset_buffer_t collected_offsets;
        // Fill the offset buffer for garbage collection.
        // Exit the loop when we either have run out of records to process or the offsets buffer is full.
        for (; record_index < record_count && collected_offsets.size() < c_offset_buffer_size; ++record_index)
        {
            const auto& log_record = txn_log->log_records[record_index];

            gaia_offset_t offset = deallocate_new_offsets ? log_record.new_offset : log_record.old_offset;

            // If no action is needed, move on to the next log record.
            if (offset.is_valid())
            {
                auto obj = offset_to_ptr(offset);

                // We do not index catalog core objects, so we can move on.
                if (is_catalog_core_object(obj->type))
                {
                    continue;
                }

                // Add the offset to the buffers and advance the buffer index.
                collected_offsets.insert(offset, obj->type);
            }
        }

        // When we reach this point, either we have 1) run out of records to iterate over or 2) the offsets buffer is now considered full.
        // We know that 2) is false when the offsets buffer is empty and there is no garbage to collect.
        // Therefore we can safely return here.
        if (collected_offsets.empty())
        {
            return;
        }

        // Garbage collect the offsets in the buffer.
        for (const auto& it : *get_indexes())
        {
            switch (it.second->type())
            {
            case catalog::index_type_t::range:
                remove_entries_with_offsets<range_index_t>(it.second.get(), collected_offsets, txn_log->begin_ts());
                break;
            case catalog::index_type_t::hash:
                remove_entries_with_offsets<hash_index_t>(it.second.get(), collected_offsets, txn_log->begin_ts());
                break;
            }
        }
    }
}

template <class T_index>
void mark_index_entries(base_index_t* base_index, gaia_txn_id_t txn_id)
{
    auto index = static_cast<T_index*>(base_index);
    index->mark_entries_committed(txn_id);
}

void index_builder_t::mark_index_entries_committed(gaia_txn_id_t txn_id)
{
    for (const auto& it : *get_indexes())
    {
        // Optimization: only mark index entries committed for UNIQUE indexes, as we only look up the flags on that path.
        if (it.second->is_unique())
        {
            switch (it.second->type())
            {
            case catalog::index_type_t::range:
                mark_index_entries<range_index_t>(it.second.get(), txn_id);
                break;
            case catalog::index_type_t::hash:
                mark_index_entries<hash_index_t>(it.second.get(), txn_id);
                break;
            }
        }
    }
}

} // namespace index
} // namespace db
} // namespace gaia
