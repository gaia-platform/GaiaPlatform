/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_fdw_adapter.hpp"

#include <fstream>
#include <sstream>

#include "catalog_core.hpp"
#include "catalog_internal.hpp"
#include "field_access.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::payload_types;

namespace gaia
{
namespace fdw
{

constexpr size_t c_invalid_field_index = -1;

unordered_map<string, gaia_type_t> adapter_t::s_map_table_name_to_container_id;

// Valid options for gaia_fdw.
const option_t c_valid_options[] = {
    // Sentinel.
    {nullptr, InvalidOid, nullptr}};

int adapter_t::s_txn_reference_count = 0;

bool validate_and_apply_option(const char* option_name, const char* value, Oid context_id)
{
    for (const option_t* option = c_valid_options; option->name; ++option)
    {
        if (option->context_id == context_id && strcmp(option->name, option_name) == 0)
        {
            // Invoke option handler callback.
            option->handler(option_name, value, context_id);
            return true;
        }
    }
    return false;
}

void append_context_option_names(Oid context_id, StringInfoData& string_info)
{
    initStringInfo(&string_info);
    for (const option_t* option = c_valid_options; option->name; ++option)
    {
        if (context_id == option->context_id)
        {
            appendStringInfo(
                &string_info,
                "%s%s",
                (string_info.len > 0) ? ", " : "",
                option->name);
        }
    }
}

void adapter_t::initialize_caches()
{
    gaia::db::begin_transaction();

    for (auto table_view : catalog_core_t::list_tables())
    {
        string table_name(table_view.name());
        vector<uint8_t> binary_schema = table_view.binary_schema();
        vector<uint8_t> serialization_template = table_view.serialization_template();

        stringstream log_message;
        log_message
            << "Loading metadata information for table `" << table_name
            << "' with type " << table_view.table_type() << "...";
        elog(LOG, log_message.str().c_str());

        auto type_information = make_shared<const type_information_t>();

        initialize_type_information_from_binary_schema(
            const_cast<type_information_t*>(type_information.get()),
            binary_schema.data(),
            binary_schema.size());

        const_cast<type_information_t*>(type_information.get())->set_serialization_template(serialization_template);

        bool result = type_cache_t::get()->set_type_information(table_view.table_type(), type_information);
        retail_assert(result, "Failed setting type_cache!");

        s_map_table_name_to_container_id.insert(make_pair(table_name, table_view.table_type()));
    }

    retail_assert(
        type_cache_t::get()->size() == s_map_table_name_to_container_id.size(),
        "Type caches were initialized to different sizes!");

    gaia::db::commit_transaction();
}

void adapter_t::begin_session()
{
    elog(LOG, "Opening COW-SE session...");

    try
    {
        gaia::db::begin_session();

        initialize_caches();
    }
    catch (exception e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Error opening COW-SE session."),
             errhint("%s", e.what())));
    }
}

void adapter_t::end_session()
{
    elog(LOG, "Closing COW-SE session...");

    try
    {
        gaia::db::end_session();
    }
    catch (exception e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Error closing COW-SE session."),
             errhint("%s", e.what())));
    }
}

bool adapter_t::is_transaction_open()
{
    retail_assert(s_txn_reference_count >= 0, "Transaction count is negative!");
    return s_txn_reference_count > 0;
}

bool adapter_t::begin_transaction()
{
    elog(DEBUG1, "Opening COW-SE transaction...");

    retail_assert(s_txn_reference_count >= 0, "Transaction count is negative!");

    bool opened_transaction = false;
    int previous_count = s_txn_reference_count++;
    if (previous_count == 0)
    {
        opened_transaction = true;
        gaia::db::begin_transaction();
    }

    elog(DEBUG1, "Txn actually opened: %s.", opened_transaction ? "true" : "false");

    return opened_transaction;
}

bool adapter_t::commit_transaction()
{
    elog(DEBUG1, "Closing COW-SE transaction...");

    retail_assert(s_txn_reference_count > 0, "Transaction count was not positive at commit time.");

    bool closed_transaction = false;
    int previous_count = s_txn_reference_count--;
    if (previous_count == 1)
    {
        closed_transaction = true;
        gaia::db::commit_transaction();
    }

    elog(DEBUG1, "Txn actually closed: %s.", closed_transaction ? "true" : "false");

    return closed_transaction;
}

bool adapter_t::is_gaia_id_name(const char* name)
{
    constexpr char c_gaia_id[] = "gaia_id";

    return strcmp(c_gaia_id, name) == 0;
}

uint64_t adapter_t::get_new_gaia_id()
{
    return gaia_ptr::generate_id();
}

// TODO: Rewrite this to iterate over the database tables and generate statements
// out of table definitions.
List* adapter_t::get_ddl_command_list(const char* server_name)
{
    List* commands = NIL;

    gaia::db::begin_transaction();

    for (auto table_view : catalog_core_t::list_tables())
    {
        // Generate DDL statement for current table and log it.
        string ddl_formatted_statement = gaia::catalog::generate_fdw_ddl(table_view.id(), server_name);
        elog(LOG, ddl_formatted_statement.c_str());

        // Copy DDL statement into a Postgres buffer.
        auto statement_buffer = reinterpret_cast<char*>(palloc(ddl_formatted_statement.size() + 1));
        strncpy(statement_buffer, ddl_formatted_statement.c_str(), ddl_formatted_statement.size() + 1);

        commands = lappend(commands, statement_buffer);
    }

    gaia::db::commit_transaction();

    return commands;
}

bool state_t::initialize(const char* table_name, size_t count_fields)
{
    m_gaia_id_field_index = c_invalid_field_index;

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
    else
    {
        return false;
    }

    retail_assert(
        count_fields == m_mapping->attribute_count,
        "Table has a different number of fields than expected!");

    m_gaia_container_id = m_mapping->gaia_container_id;

    m_indexed_accessors = reinterpret_cast<attribute_accessor_fn*>(palloc0(
        sizeof(attribute_accessor_fn) * m_mapping->attribute_count));

    m_indexed_builders = reinterpret_cast<attribute_builder_fn*>(palloc0(
        sizeof(attribute_builder_fn) * m_mapping->attribute_count));

    return true;
}

bool state_t::set_field_index(const char* field_name, size_t field_index)
{
    if (field_index >= m_mapping->attribute_count)
    {
        return false;
    }

    bool found_field = false;
    for (size_t i = 0; i < m_mapping->attribute_count; ++i)
    {
        if (strcmp(field_name, m_mapping->attributes[i].name) == 0)
        {
            found_field = true;

            m_indexed_accessors[field_index] = m_mapping->attributes[i].accessor;
            m_indexed_builders[field_index] = m_mapping->attributes[i].builder;

            if (adapter_t::is_gaia_id_name(field_name))
            {
                m_gaia_id_field_index = field_index;
            }

            break;
        }
    }

    retail_assert(m_gaia_id_field_index != c_invalid_field_index, "Failed to determine the index of gaia_id field!");
    if (m_gaia_id_field_index == c_invalid_field_index)
    {
        return false;
    }

    return found_field;
}

bool state_t::is_gaia_id_field_index(size_t field_index)
{
    return field_index == m_gaia_id_field_index;
}

bool scan_state_t::initialize(const char* table_name, size_t count_fields)
{
    if (!state_t::initialize(table_name, count_fields))
    {
        return false;
    }

    m_deserializer = m_mapping->deserializer;

    m_current_object_root = nullptr;

    return true;
}

bool scan_state_t::initialize_scan()
{
    m_current_node = gaia_ptr::find_first(m_mapping->gaia_container_id);

    return true;
}

bool scan_state_t::has_scan_ended()
{
    return !m_current_node;
}

void scan_state_t::deserialize_record()
{
    retail_assert(!has_scan_ended(), "Attempting to read record after scan has completed!");

    const void* data = m_current_node.data();
    m_current_object_root = m_deserializer(data);
}

Datum scan_state_t::extract_field_value(size_t field_index)
{
    retail_assert(
        field_index < m_mapping->attribute_count,
        "Attempting to extract information for an invalid field index!");

    if (m_current_object_root == nullptr)
    {
        deserialize_record();
    }

    retail_assert(m_current_object_root != nullptr, "Current record payload should not be empty");

    Datum field_value;

    if (is_gaia_id_field_index(field_index))
    {
        field_value = UInt64GetDatum(m_current_node.id());
    }
    else
    {
        attribute_accessor_fn accessor = m_indexed_accessors[field_index];
        field_value = accessor(m_current_object_root);
    }

    return field_value;
}

bool scan_state_t::scan_forward()
{
    retail_assert(!has_scan_ended(), "Attempting to continue scan after it has terminated!");

    m_current_node = m_current_node.find_next();

    m_current_object_root = nullptr;

    return has_scan_ended();
}

bool modify_state_t::initialize(const char* table_name, size_t count_fields)
{
    if (!state_t::initialize(table_name, count_fields))
    {
        return false;
    }

    m_initializer = m_mapping->initializer;
    m_finalizer = m_mapping->finalizer;

    flatcc_builder_init(&m_builder);

    m_has_initialized_builder = false;

    return true;
}

void modify_state_t::initialize_modify()
{
    m_initializer(&m_builder);
    m_has_initialized_builder = true;
}

void modify_state_t::set_field_value(size_t field_index, const Datum& field_value)
{
    retail_assert(!is_gaia_id_field_index(field_index), "Attempting to set value of gaia_id field!");

    attribute_builder_fn accessor = m_indexed_builders[field_index];
    accessor(&m_builder, field_value);
}

bool modify_state_t::edit_record(uint64_t gaia_id, edit_state_t edit_state)
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
                m_gaia_container_id,
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
    catch (const exception& e)
    {
        flatcc_builder_reset(&m_builder);

        if (edit_state == edit_state_t::create)
        {
            ereport(
                ERROR,
                (errcode(ERRCODE_FDW_ERROR),
                 errmsg("Error creating gaia object."),
                 errhint("%s", e.what())));
        }
        else if (edit_state == edit_state_t::update)
        {
            ereport(
                ERROR,
                (errcode(ERRCODE_FDW_ERROR),
                 errmsg("Error updating gaia object."),
                 errhint("%s", e.what())));
        }

        return false;
    }

    flatcc_builder_reset(&m_builder);

    return result;
}

bool modify_state_t::insert_record(uint64_t gaia_id)
{
    return edit_record(gaia_id, edit_state_t::create);
}

bool modify_state_t::update_record(uint64_t gaia_id)
{
    return edit_record(gaia_id, edit_state_t::update);
}

bool modify_state_t::delete_record(uint64_t gaia_id)
{
    try
    {
        auto node = gaia_ptr::open(gaia_id);
        if (!node)
        {
            elog(DEBUG1, "Node for gaia_id %ld is invalid.", gaia_id);
            return false;
        }

        elog(DEBUG1, "Calling remove() on node for gaia_id %ld.", gaia_id);
        gaia_ptr::remove(node);

        return true;
    }
    catch (const exception& e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Error deleting gaia object."),
             errhint("%s", e.what())));
    }

    return false;
}

void modify_state_t::finalize_modify()
{
    if (m_has_initialized_builder)
    {
        flatcc_builder_clear(&m_builder);
        m_has_initialized_builder = false;
    }
}

} // namespace fdw
} // namespace gaia
