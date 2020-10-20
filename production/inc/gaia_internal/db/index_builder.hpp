/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/common.hpp"
#include "base_index.hpp"
#include "db_internal_types.hpp"
#include "index.hpp"

// Opaque API for index maintenance

class index_scan_iterator_base_t;

namespace gaia
{
namespace db
{
namespace index
{

class index_builder
{
private:
    static index_record_t make_insert_record(gaia::db::gaia_locator_t locator);
    static index_record_t make_delete_record(gaia::db::gaia_locator_t locator);

    static void update_index(gaia::common::gaia_id_t index_id, index_key_t&& key, index_record_t record);

public:
    static void create_empty_index(gaia::common::gaia_id_t index_id, value_index_type_t type);
    static void update_index(gaia::common::gaia_id_t index_id, txn_log_t::log_record_t& log_record);

    static index_key_t make_key(gaia::common::gaia_id_t index_id, const uint8_t* payload);
    static bool index_exists(gaia::common::gaia_id_t index_id);

    // Initialize index from data
    static void init_index(gaia::common::gaia_id_t index_id);
};

} // namespace index
} // namespace db
} // namespace gaia
