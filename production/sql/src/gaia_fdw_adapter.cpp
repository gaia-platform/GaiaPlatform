/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gaia_fdw_adapter.hpp>

using namespace gaia::db;
using namespace gaia::fdw;

gaia_fdw_adapter_t* gaia_fdw_adapter_t::get_table_adapter(
    adapter_state_t adapter_state, const char* table_name, size_t count_accessors)
{
    gaia_fdw_adapter_t* adapter = (gaia_fdw_adapter_t *)palloc0(sizeof(gaia_fdw_adapter_t));

    adapter->m_adapter_state = adapter_state;

    return adapter->initialize(table_name, count_accessors) ? adapter : nullptr;
}

bool gaia_fdw_adapter_t::initialize(const char* table_name, size_t count_accessors)
{
    if (strcmp(table_name, "airports") == 0) {
        m_mapping = c_airport_mapping;
    } else if (strcmp(table_name, "airlines") == 0) {
        m_mapping = c_airline_mapping;
    } else if (strcmp(table_name, "routes") == 0) {
        m_mapping = c_route_mapping;
    } else if (strcmp(table_name, "event_log") == 0) {
        m_mapping = c_event_log_mapping;
    } else {
        return false;
    }

    assert(count_accessors == m_mapping.attribute_count);

    m_scan_state.deserializer = m_mapping.deserializer;

    m_scan_state.indexed_accessors = (attribute_accessor_fn*)palloc0(
        sizeof(attribute_accessor_fn) * m_mapping.attribute_count);

    m_object_root = nullptr;

    return true;
}

bool gaia_fdw_adapter_t::set_accessor_index(const char* accessor_name, size_t accessor_index)
{
    if (accessor_index >= m_mapping.attribute_count)
    {
        return false;
    }

    bool found_accessor = false;
    for (size_t i = 0; i < m_mapping.attribute_count; i++)
    {
        if (strcmp(accessor_name, m_mapping.attributes[i].name) == 0)
        {
            m_scan_state.indexed_accessors[accessor_index] = m_mapping.attributes[i].accessor;
            found_accessor = true;
            break;
        }
    }

    return found_accessor;
}

bool gaia_fdw_adapter_t::initialize_scan()
{
    assert(m_adapter_state == adapter_state_t::scan);
    if (m_adapter_state != adapter_state_t::scan)
    {
        return false;
    }

    m_scan_state.current_node = gaia_ptr::find_first(m_mapping.gaia_type_id);

    return true;
}

bool gaia_fdw_adapter_t::has_scan_ended()
{
    assert(m_adapter_state == adapter_state_t::scan);

    return !m_scan_state.current_node;
}

void gaia_fdw_adapter_t::deserialize_record()
{
    assert(!has_scan_ended());

    const void* data;
    data = m_scan_state.current_node.data();
    m_object_root = m_scan_state.deserializer(data);
}

Datum gaia_fdw_adapter_t::extract_field_value(size_t field_index)
{
    assert(m_adapter_state == adapter_state_t::scan);
    assert(field_index < m_mapping.attribute_count);

    if (m_object_root == nullptr)
    {
        deserialize_record();
    }

    assert(m_object_root != nullptr);

    attribute_accessor_fn accessor = m_scan_state.indexed_accessors[field_index];
    Datum field_value = accessor(m_object_root);
    return field_value;
}

bool gaia_fdw_adapter_t::scan_forward()
{
    assert(!has_scan_ended());

    m_scan_state.current_node = m_scan_state.current_node.find_next();

    m_object_root = nullptr;

    return has_scan_ended();
}
