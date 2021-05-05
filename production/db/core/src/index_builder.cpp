/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/index/index_builder.hpp"

#include <iostream>
#include <utility>

#include "gaia_internal/db/catalog_core.hpp"

#include "data_holder.hpp"
#include "db_helpers.hpp"
#include "field_access.hpp"
#include "hash_index.hpp"
#include "range_index.hpp"
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
void truncate_index(index_writer_t<T_index>* w, gaia_txn_id_t commit_ts)
{
    auto index = w->get_index();
    auto end = index.end();
    auto iter = index.begin();

    while (iter != end)
    {
        if (txn_metadata_t::get_commit_ts(iter->second.txn_id) < commit_ts)
        {
            iter = index.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

index_key_t index_builder::make_key(gaia_id_t index_id, gaia_type_t type_id, const uint8_t* payload)
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

    for (auto field_id : *(index_view.fields()))
    {
        field_position_t pos = field_view_t(id_to_ptr(field_id)).position();
        k.insert(payload_types::get_field_value(type_id, payload, schema->data(), schema->size(), pos));
    }

    return k;
}

index_record_t index_builder::make_insert_record(gaia::db::gaia_locator_t locator, gaia::db::gaia_offset_t offset)
{
    return index_record_t{locator, txn_metadata_t::get_current_txn_id(), offset, false};
}

index_record_t index_builder::make_delete_record(gaia::db::gaia_locator_t locator, gaia::db::gaia_offset_t offset)
{
    return index_record_t{locator, txn_metadata_t::get_current_txn_id(), offset, true};
}

bool index_builder::index_exists(common::gaia_id_t index_id)
{
    return get_indexes()->find(index_id) != get_indexes()->end();
}

void index_builder::create_empty_index(gaia_id_t index_id, index_type_t type)
{
    switch (type)
    {
    case index_type_t::range:
        get_indexes()->emplace(
            index_id,
            std::make_shared<range_index_t, gaia_id_t>(static_cast<gaia_id_t&&>(index_id)));
        break;
    case index_type_t::hash:
        get_indexes()->emplace(
            index_id,
            std::make_shared<hash_index_t, gaia_id_t>(static_cast<gaia_id_t&&>(index_id)));
        break;
    }
}

void index_builder::update_index(gaia_id_t index_id, index_key_t&& key, index_record_t record)
{
    ASSERT_PRECONDITION(get_indexes(), "Indexes not initialized");

    auto it = get_indexes()->find(index_id);

    if (it == get_indexes()->end())
    {
        throw index_not_found(index_id);
    }

    switch (it->second->type())
    {
    case index_type_t::range:
    {
        auto idx = static_cast<range_index_t*>(it->second.get());
        idx->insert_index_entry(std::move(key), record);
    }
    break;
    case index_type_t::hash:
    {
        auto idx = static_cast<hash_index_t*>(it->second.get());
        idx->insert_index_entry(std::move(key), record);
    }
    break;
    }
}

void index_builder::update_index(gaia::common::gaia_id_t index_id, gaia_type_t type_id, const txn_log_t::log_record_t& log_record)
{
    auto obj = locator_to_ptr(log_record.locator);
    uint8_t* payload = (obj) ? reinterpret_cast<uint8_t*>(obj->payload) : nullptr;

    switch (log_record.operation)
    {
    case gaia_operation_t::create:
        index_builder::update_index(
            index_id,
            index_builder::make_key(index_id, type_id, payload),
            index_builder::make_insert_record(log_record.locator, log_record.new_offset));
        break;
    case gaia_operation_t::update:
    {
        auto old_obj = offset_to_ptr(log_record.old_offset);
        auto old_payload = (old_obj) ? reinterpret_cast<uint8_t*>(old_obj->payload)
                                     : nullptr;
        index_builder::update_index(
            index_id,
            index_builder::make_key(index_id, type_id, old_payload),
            index_builder::make_delete_record(log_record.locator, log_record.old_offset));
        index_builder::update_index(
            index_id,
            index_builder::make_key(index_id, type_id, payload),
            index_builder::make_insert_record(log_record.locator, log_record.new_offset));
    }
    break;
    case gaia_operation_t::remove:
    {
        auto old_obj = offset_to_ptr(log_record.old_offset);
        auto old_payload = (old_obj) ? reinterpret_cast<uint8_t*>(old_obj->payload)
                                     : nullptr;
        index_builder::update_index(
            index_id,
            index_builder::make_key(index_id, type_id, old_payload),
            index_builder::make_delete_record(log_record.locator, log_record.old_offset));
    }
    break;
    case gaia_operation_t::clone:
        index_builder::update_index(
            index_id,
            index_builder::make_key(index_id, type_id, payload),
            index_builder::make_insert_record(log_record.locator, log_record.new_offset));
        break;
    case gaia_operation_t::noop:
        // Do nothing. This log is only a marker for the in-memory indexes.
        // It is emitted during the startup and (possibly) in future for logical replication.
        // We should not be encountering it here.
    default:
        ASSERT_UNREACHABLE("Cannot handle log type");
        return;
    }
}

void index_builder::populate_index(common::gaia_id_t index_id, common::gaia_type_t type_id, gaia_locator_t locator)
{
    auto payload = reinterpret_cast<uint8_t*>(locator_to_ptr(locator)->payload);
    update_index(index_id, make_key(index_id, type_id, payload), make_insert_record(locator, locator_to_offset(locator)));
}

void index_builder::truncate_index_to_ts(common::gaia_id_t index_id, gaia_txn_id_t commit_ts)
{
    ASSERT_PRECONDITION(get_indexes(), "Indexes not initialized");
    auto it = get_indexes()->find(index_id);

    if (it == get_indexes()->end())
    {
        throw index_not_found(index_id);
    }

    switch (it->second->type())
    {
    case index_type_t::range:
    {
        auto idx = static_cast<range_index_t*>(it->second.get());
        auto w = idx->get_writer();
        truncate_index(&w, commit_ts);
    }
    break;
    case index_type_t::hash:
    {
        auto idx = static_cast<hash_index_t*>(it->second.get());
        auto w = idx->get_writer();
        truncate_index(&w, commit_ts);
    }
    break;
    }
}

void index_builder::update_indexes_from_logs(const txn_log_t& records)
{
    for (size_t i = 0; i < records.record_count; ++i)
    {
        auto& log = records.log_records[i];
        if (!is_logical_operation(log.operation))
        {
            gaia_type_t obj_type = c_invalid_gaia_type;

            if (log.operation == gaia_operation_t::remove)
            {
                auto obj = offset_to_ptr(log.old_offset);
                ASSERT_INVARIANT(obj != nullptr, "Cannot find db object.");
                obj_type = obj->type;
            }
            else
            {
                auto obj = offset_to_ptr(log.new_offset);
                ASSERT_INVARIANT(obj != nullptr, "Cannot find db object.");
                obj_type = obj->type;
            }

            // Ignore system tables for now.
            if (obj_type >= c_system_table_reserved_range_start)
            {
                // Flag the type_id_mapping cache to clear if any changes
                if (obj_type == static_cast<gaia_type_t>(system_table_type_t::catalog_gaia_table))
                {
                    type_id_mapping_t::instance().clear();
                }
                continue;
            }

            gaia_id_t type_record_id = type_id_mapping_t::instance().get_record_id(obj_type);

            if (type_record_id == c_invalid_gaia_id)
            {
                // Non-catalogued type, might happen in tests
                continue;
            }

            for (auto idx : catalog_core_t::list_indexes(type_record_id))
            {
                index::index_builder::update_index(idx.id(), obj_type, log);
            }
        }
    }
}

} // namespace index
} // namespace db
} // namespace gaia
