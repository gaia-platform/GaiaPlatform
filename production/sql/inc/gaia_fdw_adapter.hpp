/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

// All Postgres headers and function declarations must have C linkage.
extern "C" {

#include "postgres.h"

#include "nodes/pg_list.h"

} // extern "C"


#include "airport_demo_type_mapping.hpp"
#include "system_catalog_type_mapping.hpp"

namespace gaia
{
namespace fdw
{

const char* const c_gaia_id = "gaia_id";

// Adapter state.
enum class adapter_state_t: int8_t
{
    none = 0,

    scan = 1,
    modify = 2,
};

enum class edit_state_t: int8_t
{
    none = 0,

    create = 1,
    update = 2,
};

class gaia_fdw_adapter_t
{
protected:

    // Do not allow copies to be made;
    // disable copy constructor and assignment operator.
    gaia_fdw_adapter_t(const gaia_fdw_adapter_t&) = delete;
    gaia_fdw_adapter_t& operator=(const gaia_fdw_adapter_t&) = delete;

    // gaia_fdw_adapter_t is a singleton, so its constructor is not public.
    gaia_fdw_adapter_t() = default;

    bool initialize(const char* table_name, size_t count_accessors);
    void deserialize_record();

    bool edit_record(uint64_t gaia_id, edit_state_t edit_state);

public:

    static bool is_transaction_open();
    static bool begin_transaction();
    static bool commit_transaction();

    static uint64_t get_new_gaia_id();

    static List* get_ddl_command_list(const char* server_name);

    static gaia_fdw_adapter_t* get_table_adapter(
        adapter_state_t adapter_state, const char* table_name, size_t count_accessors);

    bool set_accessor_index(const char* accessor_name, size_t accessor_index);

    // Scan API.
    bool initialize_scan();
    bool has_scan_ended();
    Datum extract_field_value(size_t field_index);
    bool scan_forward();

    // Modify API.
    void initialize_modify();
    bool is_gaia_id_field_index(size_t field_index);
    void set_field_value(size_t field_index, const Datum& field_value);
    bool insert_record(uint64_t gaia_id);
    bool update_record(uint64_t gaia_id);
    bool delete_record(uint64_t gaia_id);
    void finalize_modify();

protected:

    // HACKHACK: global counter to simulate nested transactions. Because a DELETE
    // plan is nested within a scan, committing the write txn will invalidate the
    // read txn. We get around this by using a refcount to track the txn nesting
    // state, so we only open a txn when the counter is initially incremented from 0
    // and only commit a txn when the counter is decremented to 0. An unsynchronized
    // global counter is ok since there's no concurrency within a postgres backend.
    //
    // Use signed int so we can assert it is non-negative.
    static int s_transaction_reference_count;

    adapter_state_t m_adapter_state;

    relation_attribute_mapping_t m_mapping;

    gaia_fdw_state_t m_state;
};

}
}
