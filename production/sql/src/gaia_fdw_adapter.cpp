/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gaia_fdw_adapter.hpp>

#include <fstream>

#include "array_size.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::fdw;

const int c_invalid_index = -1;

int gaia_fdw_adapter_t::s_transaction_reference_count = 0;

bool gaia_fdw_adapter_t::is_transaction_open()
{
    assert(s_transaction_reference_count >= 0);
    return s_transaction_reference_count > 0;
}

bool gaia_fdw_adapter_t::begin_transaction()
{
    elog(DEBUG1, "Opening COW-SE transaction...");

    assert(s_transaction_reference_count >= 0);

    bool opened_transaction = false;
    int previous_count = s_transaction_reference_count++;
    if (previous_count == 0)
    {
        opened_transaction = true;
        gaia::db::begin_transaction();
    }

    elog(DEBUG1, "Txn actually opened: %s.", opened_transaction ? "true" : "false");

    return opened_transaction;
}

bool gaia_fdw_adapter_t::commit_transaction()
{
    elog(DEBUG1, "Closing COW-SE transaction...");

    assert(s_transaction_reference_count > 0);

    bool closed_transaction = false;
    int previous_count = s_transaction_reference_count--;
    if (previous_count == 1)
    {
        closed_transaction = true;
        gaia::db::commit_transaction();
    }

    elog(DEBUG1, "Txn actually closed: %s.", closed_transaction ? "true" : "false");

    return closed_transaction;
}

// Generate random gaia_id for INSERTs using /dev/urandom.
// NB: because this is a 64-bit value, we will expect collisions
// as the number of generated keys approaches 2^32! This is just
// a temporary hack until the gaia_id type becomes a 128-bit GUID.
uint64_t gaia_fdw_adapter_t::get_new_gaia_id()
{
    ifstream urandom("/dev/urandom", ios::in | ios::binary);
    if (!urandom)
    {
        elog(ERROR, "Failed to open /dev/urandom.");
        return 0;
    }

    uint64_t random_value;
    urandom.read(reinterpret_cast<char*>(&random_value), sizeof(random_value));
    if (!urandom)
    {
        urandom.close();
        elog(ERROR, "Failed to read from /dev/urandom.");
        return 0;
    }

    urandom.close();

    // Generating 0 should be statistically impossible.
    assert(random_value != 0);
    return random_value;
}

List* gaia_fdw_adapter_t::get_ddl_command_list(const char* server_name)
{
    List* commands = NIL;

    const char* ddl_formatted_statements[] =
    {
        c_airport_ddl_stmt_fmt,
        c_airline_ddl_stmt_fmt,
        c_route_ddl_stmt_fmt,
        c_event_log_ddl_stmt_fmt,
    };

    for (size_t i = 0; i < array_size(ddl_formatted_statements); i++)
    {
        // Length of format string + length of server name - 2 chars for format
        // specifier + 1 char for null terminator.
        size_t statement_length = strlen(ddl_formatted_statements[i])
            + strlen(server_name) - sizeof("%s") + 1;
        char* statement_buffer = (char*)palloc(statement_length);

        // sprintf returns number of chars written, not including null
        // terminator.
        if (sprintf(statement_buffer, ddl_formatted_statements[i], server_name)
            != (int)(statement_length - 1))
        {
            elog(ERROR, "Failed to format statement '%s' with server name '%s'.",
                ddl_formatted_statements[i], server_name);
        }

        commands = lappend(commands, statement_buffer);
    }

    return commands;
}

gaia_fdw_scan_state_t* gaia_fdw_adapter_t::get_scan_state(
    const char* table_name, size_t count_accessors)
{
    gaia_fdw_scan_state_t* scan_state = (gaia_fdw_scan_state_t*)palloc0(sizeof(gaia_fdw_scan_state_t));

    return scan_state->initialize(table_name, count_accessors) ? scan_state : nullptr;
}

gaia_fdw_modify_state_t* gaia_fdw_adapter_t::get_modify_state(
    const char* table_name, size_t count_accessors)
{
    gaia_fdw_modify_state_t* modify_state = (gaia_fdw_modify_state_t*)palloc0(sizeof(gaia_fdw_modify_state_t));

    return modify_state->initialize(table_name, count_accessors) ? modify_state : nullptr;
}

bool gaia_fdw_state_t::initialize(const char* table_name, size_t count_accessors)
{
    if (strcmp(table_name, "airports") == 0)
    {
        m_mapping = &c_airport_mapping;
    }
    else if (strcmp(table_name, "airlines") == 0)
    {
        m_mapping = &c_airline_mapping;
    }
    else if (strcmp(table_name, "routes") == 0)
    {
        m_mapping = &c_route_mapping;
    }
    else if (strcmp(table_name, "event_log") == 0)
    {
        m_mapping = &c_event_log_mapping;
    }
    else
    {
        return false;
    }

    assert(count_accessors == m_mapping->attribute_count);

    return true;
}

bool gaia_fdw_scan_state_t::initialize(const char* table_name, size_t count_accessors)
{
    if (!gaia_fdw_state_t::initialize(table_name, count_accessors))
    {
        return false;
    }

    m_deserializer = m_mapping->deserializer;

    m_indexed_accessors = (attribute_accessor_fn*)palloc0(
        sizeof(attribute_accessor_fn) * m_mapping->attribute_count);

    m_current_object_root = nullptr;

    return true;
}

bool gaia_fdw_scan_state_t::set_accessor_index(const char* accessor_name, size_t accessor_index)
{
    if (accessor_index >= m_mapping->attribute_count)
    {
        return false;
    }

    bool found_accessor = false;
    for (size_t i = 0; i < m_mapping->attribute_count; i++)
    {
        if (strcmp(accessor_name, m_mapping->attributes[i].name) == 0)
        {
            found_accessor = true;
            m_indexed_accessors[accessor_index] = m_mapping->attributes[i].accessor;
            break;
        }
    }

    return found_accessor;
}

bool gaia_fdw_scan_state_t::initialize_scan()
{
    m_current_node = gaia_ptr::find_first(m_mapping->gaia_type_id);

    return true;
}

bool gaia_fdw_scan_state_t::has_scan_ended()
{
    return !m_current_node;
}

void gaia_fdw_scan_state_t::deserialize_record()
{
    assert(!has_scan_ended());

    const void* data;
    data = m_current_node.data();
    m_current_object_root = m_deserializer(data);
}

Datum gaia_fdw_scan_state_t::extract_field_value(size_t field_index)
{
    assert(field_index < m_mapping->attribute_count);

    if (m_current_object_root == nullptr)
    {
        deserialize_record();
    }

    assert(m_current_object_root != nullptr);

    attribute_accessor_fn accessor = m_indexed_accessors[field_index];
    Datum field_value = accessor(m_current_object_root);
    return field_value;
}

bool gaia_fdw_scan_state_t::scan_forward()
{
    assert(!has_scan_ended());

    m_current_node = m_current_node.find_next();

    m_current_object_root = nullptr;

    return has_scan_ended();
}

bool gaia_fdw_modify_state_t::initialize(const char* table_name, size_t count_accessors)
{
    if (!gaia_fdw_state_t::initialize(table_name, count_accessors))
    {
        return false;
    }

    m_pk_attr_idx = c_invalid_index;

    m_gaia_type_id = m_mapping->gaia_type_id;
    m_initializer = m_mapping->initializer;
    m_finalizer = m_mapping->finalizer;

    flatcc_builder_init(&m_builder);

    m_indexed_builders = (attribute_builder_fn*)palloc0(
        sizeof(attribute_builder_fn) * m_mapping->attribute_count);

    m_has_initialized_builder = false;

    return true;
}

bool gaia_fdw_modify_state_t::set_builder_index(const char* builder_name, size_t builder_index)
{
    if (builder_index >= m_mapping->attribute_count)
    {
        return false;
    }

    bool found_builder = false;
    for (size_t i = 0; i < m_mapping->attribute_count; i++)
    {
        if (strcmp(builder_name, m_mapping->attributes[i].name) == 0)
        {
            found_builder = true;

            m_indexed_builders[builder_index] = m_mapping->attributes[i].builder;

            if (strcmp(builder_name, c_gaia_id) == 0)
            {
                m_pk_attr_idx = builder_index;
            }

            break;
        }
    }

    assert(m_pk_attr_idx != c_invalid_index);
    if (m_pk_attr_idx == c_invalid_index)
    {
        return false;
    }

    return found_builder;
}

void gaia_fdw_modify_state_t::initialize_modify()
{
    m_initializer(&m_builder);
    m_has_initialized_builder = true;
}

bool gaia_fdw_modify_state_t::is_gaia_id_field_index(size_t field_index)
{
    return field_index == (size_t)m_pk_attr_idx;
}

void gaia_fdw_modify_state_t::set_field_value(size_t field_index, const Datum& field_value)
{
    attribute_builder_fn accessor = m_indexed_builders[field_index];
    accessor(&m_builder, field_value);
}

bool gaia_fdw_modify_state_t::edit_record(uint64_t gaia_id, edit_state_t edit_state)
{
    m_finalizer(&m_builder);

    size_t record_payload_size;
    const void* record_payload = flatcc_builder_get_direct_buffer(
        &m_builder, &record_payload_size);

    bool result = false;
    try
    {
        if (edit_state == edit_state_t::create)
        {
            gaia_ptr::create(
                gaia_id,
                m_gaia_type_id,
                record_payload_size,
                record_payload);
        }
        else if (edit_state == edit_state_t::update)
        {
            auto node = gaia_ptr::open(gaia_id);
            node.update_payload(record_payload_size, record_payload);
        }

        result = true;
    }
    catch (const std::exception& e)
    {
        flatcc_builder_reset(&m_builder);

        if (edit_state == edit_state_t::create)
        {
            ereport(ERROR,
                (errcode(ERRCODE_FDW_ERROR),
                errmsg("Error creating gaia object."),
                errhint(e.what())));
        }
        else if (edit_state == edit_state_t::update)
        {
            ereport(ERROR,
                (errcode(ERRCODE_FDW_ERROR),
                errmsg("Error updating gaia object."),
                errhint(e.what())));
        }

        return false;
    }

    flatcc_builder_reset(&m_builder);

    return result;
}

bool gaia_fdw_modify_state_t::insert_record(uint64_t gaia_id)
{
    return edit_record(gaia_id, edit_state_t::create);
}

bool gaia_fdw_modify_state_t::update_record(uint64_t gaia_id)
{
    return edit_record(gaia_id, edit_state_t::update);
}

bool gaia_fdw_modify_state_t::delete_record(uint64_t gaia_id)
{
    try
    {
        auto node = gaia_ptr::open(gaia_id);
        if (!node)
        {
            elog(DEBUG1, "Node for gaia_id %d is invalid.", gaia_id);
            return false;
        }

        elog(DEBUG1, "Calling remove() on node for gaia_id %d.", gaia_id);
        gaia_ptr::remove(node);

        return true;
    }
    catch (const std::exception& e)
    {
        ereport(ERROR,
            (errcode(ERRCODE_FDW_ERROR),
            errmsg("Error deleting gaia object."),
            errhint(e.what())));
    }

    return false;
}

void gaia_fdw_modify_state_t::finalize_modify()
{
    if (m_has_initialized_builder)
    {
        flatcc_builder_clear(&m_builder);
        m_has_initialized_builder = false;
    }
}
