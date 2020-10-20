/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "index_builder.hpp"

#include <iostream>
#include <utility>

#include "catalog_core.hpp"
#include "data_holder.hpp"
#include "hash_index.hpp"
#include "range_index.hpp"
#include "db_helpers.hpp"
#include "db_txn.hpp"

using namespace gaia::common;

namespace gaia
{
namespace db
{
namespace index
{

index_key_t index_builder::make_key(gaia_id_t index_id, const uint8_t* payload)
{
    retail_assert(payload, "Cannot compute key on null payloads.");
    index_key_t k;

    //TODO
    return k;
}

index_record_t index_builder::make_insert_record(gaia::db::gaia_locator_t locator)
{
    return index_record_t{locator, get_txn_id(), locator_to_offset(locator), false};
}

index_record_t index_builder::make_delete_record(gaia::db::gaia_locator_t locator)
{
    return index_record_t{locator, get_txn_id(), locator_to_offset(locator), true};
}

void index_builder::create_empty_index(gaia_id_t index_id, value_index_type_t type)
{
    retail_assert(get_indexes(), "Indexes not initialized");

    switch (type)
    {
    case range:
        get_indexes()->emplace(
            index_id,
            std::make_shared<range_index_t, gaia_id_t>(static_cast<gaia_id_t&&>(index_id)));
        break;
    case hash:
        get_indexes()->emplace(
            index_id,
            std::make_shared<hash_index_t, gaia_id_t>(static_cast<gaia_id_t&&>(index_id)));
        break;
    }
}

void index_builder::update_index(gaia_id_t index_id, index_key_t&& key, index_record_t record)
{
    retail_assert(get_indexes(), "Indexes not initialized");

    auto it = get_indexes()->find(index_id);

    if (it == get_indexes()->end())
    {
        throw index_not_found(index_id);
    }

    switch (it->second->type())
    {
    case range:
    {
        auto idx = static_cast<range_index_t*>(it->second.get());
        idx->insert_index_entry(std::move(key), record);
    }
    break;
    case hash:
    {
        auto idx = static_cast<hash_index_t*>(it->second.get());
        idx->insert_index_entry(std::move(key), record);
    }
    break;
    }
}

void index_builder::update_index(gaia::common::gaia_id_t index_id, txn_log_t::log_record_t& log_record)
{
    auto obj = locator_to_ptr(log_record.locator);
    auto payload = (obj) ? obj->payload : nullptr;

    switch (log_record.operation)
    {
    case gaia_operation_t::create:
        index_builder::update_index(
            index_id,
            index_builder::make_key(index_id, reinterpret_cast<const uint8_t*>(payload)),
            index_builder::make_insert_record(log_record.locator));
        break;
    case gaia_operation_t::update:
    {
        auto old_obj = offset_to_ptr(log_record.old_offset);
        index_builder::update_index(
            index_id,
            index_builder::make_key(index_id, reinterpret_cast<const uint8_t*>(old_obj->payload)),
            index_builder::make_delete_record(log_record.locator));
        index_builder::update_index(
            index_id,
            index_builder::make_key(index_id, reinterpret_cast<const uint8_t*>(payload)),
            index_builder::make_insert_record(log_record.locator));
    }
    break;
    case gaia_operation_t::remove:
    {
        auto old_obj = offset_to_ptr(log_record.old_offset);
        index_builder::update_index(
            index_id,
            index_builder::make_key(index_id, reinterpret_cast<const uint8_t*>(old_obj->payload)),
            index_builder::make_delete_record(log_record.locator));
    }
    break;
    case gaia_operation_t::clone:
        index_builder::update_index(
            index_id,
            index_builder::make_key(index_id, reinterpret_cast<const uint8_t*>(payload)),
            index_builder::make_insert_record(log_record.locator));
        break;
    default:
        retail_assert(false, "Cannot handle log type");
    }
}

void index_builder::init_index(gaia_id_t index_id)
{
    retail_assert(get_indexes(), "Indexes not initialized");

    // get table
}

} // namespace index
} // namespace db
} // namespace gaia
