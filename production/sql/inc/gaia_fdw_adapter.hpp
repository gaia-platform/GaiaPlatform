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

    static gaia_fdw_adapter_t* get_table_adapter(
        adapter_state_t adapter_state, const char* table_name, size_t count_accessors);

    static uint64_t get_new_gaia_id();

    static List* get_ddl_command_list(const char* server_name);

    bool set_accessor_index(const char* accessor_name, size_t accessor_index);

    bool initialize_scan();
    bool has_scan_ended();
    Datum extract_field_value(size_t field_index);
    bool scan_forward();

    void initialize_modify();
    bool is_gaia_id_field_index(size_t field_index);
    void set_field_value(size_t field_index, const Datum& field_value);
    bool insert_record(uint64_t gaia_id);
    bool update_record(uint64_t gaia_id);
    bool delete_record(uint64_t gaia_id);
    void finalize_modify();

protected:

    adapter_state_t m_adapter_state;

    relation_attribute_mapping_t m_mapping;

    gaia_fdw_scan_state_t m_scan_state;
    const void* m_object_root;

    gaia_fdw_modify_state_t m_modify_state;
    bool m_has_initialized_modify_builder;
};

}
}
