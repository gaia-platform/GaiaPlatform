/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

#include "airport_demo_type_mapping.hpp"
#include "system_catalog_type_mapping.hpp"

namespace gaia
{
namespace fdw
{

// Adapter state.
enum class adapter_state_t: int8_t
{
    none = 0,

    scan = 1,
    modify = 2,
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

public:

    static gaia_fdw_adapter_t* get_table_adapter(
        adapter_state_t adapter_state, const char* table_name, size_t count_accessors);

    bool set_accessor_index(const char* accessor_name, size_t accessor_index);

    bool initialize_scan();
    bool has_scan_ended();
    Datum extract_field_value(size_t field_index);
    bool scan_forward();

protected:

    adapter_state_t m_adapter_state;

    relation_attribute_mapping_t m_mapping;

    gaia_fdw_scan_state_t m_scan_state;
    const void* m_object_root;
};

}
}
