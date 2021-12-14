/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/common.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/db/catalog_core.hpp"

#include "base_index.hpp"
#include "data_buffer.hpp"
#include "db_internal_types.hpp"
#include "db_object_helpers.hpp"
#include "index.hpp"

class index_scan_iterator_base_t;

namespace gaia
{
namespace db
{
namespace index
{

/**
 * Opaque API for index maintenance.
 **/

class index_builder_t
{
public:
    static bool index_exists(common::gaia_id_t index_id);
    static indexes_t::iterator create_empty_index(const index_view_t& index_view, bool skip_catalog_integrity_check = false);
    static void drop_index(const index_view_t& index_view);
    static void update_index(
        common::gaia_id_t index_id, common::gaia_type_t type_id, const txn_log_t::log_record_t& log_record, bool allow_create_empty = false);

    static index_key_t make_key(common::gaia_id_t index_id, common::gaia_type_t type_id, const uint8_t* payload);
    static void serialize_key(const index_key_t& key, payload_types::data_write_buffer_t& buffer);
    static index_key_t deserialize_key(common::gaia_id_t index_id, payload_types::data_read_buffer_t& buffer);

    static void populate_index(common::gaia_id_t index_id, common::gaia_type_t type, gaia_locator_t locator);
    static void update_indexes_from_txn_log(const txn_log_t& records, bool skip_catalog_integrity_check, bool allow_create_empty = false);
    static void gc_indexes_from_txn_log(const txn_log_t& records, bool deallocate_new_offsets);
    static void mark_index_entries_committed(gaia_txn_id_t txn_id);

private:
    static index_record_t make_record(gaia_locator_t locator, gaia_offset_t offset, index_record_operation_t operation);

    static void update_index(common::gaia_id_t index_id, index_key_t&& key, index_record_t record, bool allow_create_empty = false);
};

} // namespace index
} // namespace db
} // namespace gaia
