/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/db/index_builder.hpp"

#include <iostream>
#include <utility>

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

template <class T_index>
/*
w - the writer guard for the index meant to be truncated.
commit_ts - the cutoff timestamp below which we remove committed transactions (inclusive).
*/
void truncate_index(index_writer_guard_t<T_index>& w, gaia_txn_id_t commit_ts)
{
    auto index = w.get_index();
    auto end = index.end();
    auto iter = index.begin();

    while (iter != end)
    {
        auto curr_iter = iter++;
        auto ts = transactions::txn_metadata_t::get_commit_ts_from_begin_ts(curr_iter->second.txn_id);

        // Ignore invalid txn_ids, txn could be in-flight at the point of testing.
        if (ts <= commit_ts && ts != c_invalid_gaia_txn_id)
        {
            // Only erase entry if each key contains at least one additional entry with the same key.
            if (iter != end && curr_iter->first == iter->first)
            {
                iter = index.erase(curr_iter);
            }
        }
    }
}

index_key_t index_builder_t::make_key(gaia_id_t index_id, gaia_type_t type_id, const uint8_t* payload)
{
    ASSERT_PRECONDITION(payload, "Cannot compute key on null payloads.");

    index_key_t k;
    gaia_id_t type_record_id = type_id_mapping_t::instance().get_record_id(type_id);
    ASSERT_INVARIANT(
        type_record_id != c_invalid_gaia_id,
        "The type '" + std::to_string(type_id) + "' does not exist in the catalog.");
    auto table = catalog_core_t::get_table(type_record_id);
    auto schema = table.binary_schema();
    auto index_view = index_view_t(id_to_ptr(index_id));

    const auto& fields = *(index_view.fields());
    for (auto field_id : fields)
    {
        field_position_t pos = field_view_t(id_to_ptr(field_id)).position();
        k.insert(payload_types::get_field_value(type_id, payload, schema->data(), schema->size(), pos));
    }

    return k;
}

void index_builder_t::serialize_key(const index_key_t& key, data_write_buffer_t& buffer)
{
    for (const auto& k : key.values())
    {
        k.serialize(buffer);
    }
}

index_key_t index_builder_t::deserialize_key(common::gaia_id_t index_id, data_read_buffer_t& buffer)
{
    ASSERT_PRECONDITION(index_id != c_invalid_gaia_id, "Invalid gaia id.");

    index_key_t k;
    auto index_view = index_view_t(id_to_ptr(index_id));

    const auto& fields = *(index_view.fields());
    for (auto field_id : fields)
    {
        data_type_t type = field_view_t(id_to_ptr(field_id)).data_type();
        k.insert(payload_types::data_holder_t(buffer, gaia_to_fb_type(type)));
    }

    return k;
}

index_record_t index_builder_t::make_insert_record(gaia::db::gaia_locator_t locator, gaia::db::gaia_offset_t offset)
{
    bool is_delete = false;
    return index_record_t{get_current_txn_id(), offset, locator, is_delete};
}

index_record_t index_builder_t::make_delete_record(gaia::db::gaia_locator_t locator, gaia::db::gaia_offset_t offset)
{
    bool is_delete = true;
    return index_record_t{get_current_txn_id(), offset, locator, is_delete};
}

bool index_builder_t::index_exists(common::gaia_id_t index_id)
{
    return get_indexes()->find(index_id) != get_indexes()->end();
}

indexes_t::iterator index_builder_t::create_empty_index(gaia_id_t index_id, catalog::index_type_t type)
{
    switch (type)
    {
    case catalog::index_type_t::range:
        return get_indexes()->emplace(
                                index_id,
                                std::make_shared<range_index_t, gaia_id_t>(static_cast<gaia_id_t&&>(index_id)))
            .first;
        break;
    case catalog::index_type_t::hash:
        return get_indexes()->emplace(
                                index_id,
                                std::make_shared<hash_index_t, gaia_id_t>(static_cast<gaia_id_t&&>(index_id)))
            .first;
        break;
    }
}

void index_builder_t::update_index(gaia_id_t index_id, index_key_t&& key, index_record_t record)
{
    ASSERT_PRECONDITION(get_indexes(), "Indexes not initialized");

    auto it = get_indexes()->find(index_id);

    // Not found, need to do a catalog lookup to bootstrap the index
    if (it == get_indexes()->end())
    {
        auto index_view = index_view_t(id_to_ptr(index_id));
        it = create_empty_index(index_id, index_view.type());
    }

    switch (it->second->type())
    {
    case catalog::index_type_t::range:
    {
        auto index = static_cast<range_index_t*>(it->second.get());
        index->insert_index_entry(std::move(key), record);
    }
    break;
    case catalog::index_type_t::hash:
    {
        auto index = static_cast<hash_index_t*>(it->second.get());
        index->insert_index_entry(std::move(key), record);
    }
    break;
    }
}

void index_builder_t::update_index(gaia::common::gaia_id_t index_id, gaia_type_t type_id, const txn_log_t::log_record_t& log_record)
{
    auto obj = offset_to_ptr(log_record.new_offset);
    uint8_t* payload = (obj) ? reinterpret_cast<uint8_t*>(obj->payload) : nullptr;

    switch (log_record.operation)
    {
    case gaia_operation_t::create:
    {
        index_builder_t::update_index(
            index_id,
            index_builder_t::make_key(index_id, type_id, payload),
            index_builder_t::make_insert_record(log_record.locator, log_record.new_offset));
        break;
    }
    case gaia_operation_t::update:
    {
        auto old_obj = offset_to_ptr(log_record.old_offset);
        auto old_payload = (old_obj) ? reinterpret_cast<uint8_t*>(old_obj->payload)
                                     : nullptr;
        index_builder_t::update_index(
            index_id,
            index_builder_t::make_key(index_id, type_id, old_payload),
            index_builder_t::make_delete_record(log_record.locator, log_record.old_offset));
        index_builder_t::update_index(
            index_id,
            index_builder_t::make_key(index_id, type_id, payload),
            index_builder_t::make_insert_record(log_record.locator, log_record.new_offset));
    }
    break;
    case gaia_operation_t::remove:
    {
        auto old_obj = offset_to_ptr(log_record.old_offset);
        auto old_payload = (old_obj) ? reinterpret_cast<uint8_t*>(old_obj->payload)
                                     : nullptr;
        index_builder_t::update_index(
            index_id,
            index_builder_t::make_key(index_id, type_id, old_payload),
            index_builder_t::make_delete_record(log_record.locator, log_record.old_offset));
    }
    break;
    case gaia_operation_t::clone:
        index_builder_t::update_index(
            index_id,
            index_builder_t::make_key(index_id, type_id, payload),
            index_builder_t::make_insert_record(log_record.locator, log_record.new_offset));
        break;
    default:
        ASSERT_UNREACHABLE("Cannot handle log type");
        return;
    }
}

void index_builder_t::populate_index(common::gaia_id_t index_id, common::gaia_type_t type_id, gaia_locator_t locator)
{
    auto payload = reinterpret_cast<uint8_t*>(locator_to_ptr(locator)->payload);
    update_index(index_id, make_key(index_id, type_id, payload), make_insert_record(locator, locator_to_offset(locator)));
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

void index_builder_t::update_indexes_from_logs(const txn_log_t& records, bool skip_catalog_integrity_check)
{
    for (size_t i = 0; i < records.record_count; ++i)
    {
        auto& log_record = records.log_records[i];
        gaia_type_t obj_type = c_invalid_gaia_type;

        if (log_record.operation == gaia_operation_t::remove)
        {
            auto obj = offset_to_ptr(log_record.old_offset);
            ASSERT_INVARIANT(obj != nullptr, "Cannot find db object.");
            obj_type = obj->type;
        }
        else
        {
            auto obj = offset_to_ptr(log_record.new_offset);
            ASSERT_INVARIANT(obj != nullptr, "Cannot find db object.");
            obj_type = obj->type;
        }

        // Flag the type_id_mapping cache to clear if any changes in the schema are detected.
        if (obj_type == static_cast<gaia_type_t>(system_table_type_t::catalog_gaia_table))
        {
            type_id_mapping_t::instance().clear();
        }

        gaia_id_t type_record_id = type_id_mapping_t::instance().get_record_id(obj_type);

        // System tables are not indexed.
        // Skip if catalog verification disabled and type not found in the catalog.
        if (is_system_object(obj_type) || (skip_catalog_integrity_check && type_record_id == c_invalid_gaia_id))
        {
            continue;
        }

        for (const auto& index : catalog_core_t::list_indexes(type_record_id))
        {
            index::index_builder_t::update_index(index.id(), obj_type, log_record);
        }
    }
}

} // namespace index
} // namespace db
} // namespace gaia
