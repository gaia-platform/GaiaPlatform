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

unordered_map<string, pair<gaia_id_t, gaia_type_t>> adapter_t::s_map_table_name_to_ids;

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

Datum convert_to_datum(const data_holder_t& value)
{
    switch (value.type)
    {
    case reflection::Bool:
    case reflection::Byte:
    case reflection::UByte:
    case reflection::Short:
    case reflection::UShort:
        return Int16GetDatum(static_cast<int16_t>(value.hold.integer_value));

    case reflection::Int:
    case reflection::UInt:
        return Int32GetDatum(static_cast<int32_t>(value.hold.integer_value));

    case reflection::Long:
    case reflection::ULong:
        return Int64GetDatum(value.hold.integer_value);

    case reflection::Float:
        return Float4GetDatum(static_cast<float>(value.hold.float_value));

    case reflection::Double:
        return Float8GetDatum(value.hold.float_value);

    case reflection::String:
    {
        size_t str_len = strlen(value.hold.string_value);
        size_t text_len = str_len + VARHDRSZ;
        text* t = reinterpret_cast<text*>(palloc(text_len));

        SET_VARSIZE(t, text_len); // NOLINT (macro expansion)
        memcpy(VARDATA(t), value.hold.string_value, str_len); // NOLINT (macro expansion)

        return CStringGetDatum(t); // NOLINT (macro expansion)
    }

    default:
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("An FDW internal error was detected in convert_to_datum()!"),
             errhint("Unhandled data_holder_t type '%d'.", value.type)));
    }
}

reflection::BaseType convert_to_reflection_type(data_type_t type)
{
    switch (type)
    {
    case data_type_t::e_bool:
        return reflection::Bool;
    case data_type_t::e_uint8:
        return reflection::UByte;
    case data_type_t::e_int8:
        return reflection::Byte;
    case data_type_t::e_uint16:
        return reflection::UShort;
    case data_type_t::e_int16:
        return reflection::Short;
    case data_type_t::e_uint32:
        return reflection::UInt;
    case data_type_t::e_int32:
        return reflection::Int;
    case data_type_t::e_uint64:
        return reflection::ULong;
    case data_type_t::e_int64:
        return reflection::Long;
    case data_type_t::e_references:
        return reflection::ULong;
    case data_type_t::e_float:
        return reflection::Float;
    case data_type_t::e_double:
        return reflection::Double;
    case data_type_t::e_string:
        return reflection::String;

    default:
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("An FDW internal error was detected in convert_to_reflection_type()!"),
             errhint("Unhandled data_type_t '%d'.", type)));
    }
}

data_holder_t convert_to_data_holder(const Datum& value, data_type_t value_type)
{
    data_holder_t data_holder;

    data_holder.type = convert_to_reflection_type(value_type);

    switch (value_type)
    {
    case data_type_t::e_bool:
    case data_type_t::e_uint8:
    case data_type_t::e_int8:
    case data_type_t::e_uint16:
    case data_type_t::e_int16:
        data_holder.hold.integer_value = DatumGetInt16(value);
        break;

    case data_type_t::e_uint32:
    case data_type_t::e_int32:
        data_holder.hold.integer_value = DatumGetInt32(value);
        break;

    case data_type_t::e_uint64:
    case data_type_t::e_int64:
    case data_type_t::e_references:
        data_holder.hold.integer_value = DatumGetInt64(value);
        break;

    case data_type_t::e_float:
        data_holder.hold.float_value = DatumGetFloat4(value);
        break;

    case data_type_t::e_double:
        data_holder.hold.float_value = DatumGetFloat8(value);
        break;

    case data_type_t::e_string:
        data_holder.hold.string_value = TextDatumGetCString(value); // NOLINT (macro expansion)
        break;

    default:
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("An FDW internal error was detected in convert_to_data_holder()!"),
             errhint("Unhandled data_type_t '%d'.", value_type)));
    }

    return data_holder;
}

void adapter_t::initialize_caches()
{
    gaia::db::begin_transaction();

    for (auto table_view : catalog_core_t::list_tables())
    {
        string table_name(table_view.name());
        vector<uint8_t> binary_schema = table_view.binary_schema();
        vector<uint8_t> serialization_template = table_view.serialization_template();

        elog(
            LOG, "Loading metadata information for table `%s' with type '%ld' and id '%ld'...",
            table_view.name(), table_view.table_type(), table_view.id());

        auto type_information = make_unique<type_information_t>();

        initialize_type_information_from_binary_schema(
            type_information.get(),
            binary_schema.data(),
            binary_schema.size());

        type_information.get()->set_serialization_template(serialization_template);

        bool result = type_cache_t::get()->set_type_information(table_view.table_type(), type_information);
        if (result == false)
        {
            ereport(
                ERROR,
                (errcode(ERRCODE_FDW_ERROR),
                 errmsg("Failed to set type information in type_cache_t!")));
        }

        s_map_table_name_to_ids.insert(make_pair(
            table_name,
            make_pair(table_view.id(), table_view.table_type())));
    }

    if (type_cache_t::get()->size() != s_map_table_name_to_ids.size())
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Inconsistent initialization of caches!"),
             errhint(
                 "type_cache_t has size '%ld', but s_map_table_name_to_container_id has size '%ld'.",
                 type_cache_t::get()->size(),
                 s_map_table_name_to_ids.size())));
    }

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
    catch (const exception& e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Error opening COW-SE session."),
             errhint("Exception: %s.", e.what())));
    }
}

void adapter_t::end_session()
{
    elog(LOG, "Closing COW-SE session...");

    try
    {
        gaia::db::end_session();
    }
    catch (const exception& e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Error closing COW-SE session."),
             errhint("Exception: %s.", e.what())));
    }
}

bool adapter_t::is_transaction_open()
{
    if (s_txn_reference_count < 0)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("A negative transaction count was detected in is_transaction_open())!"),
             errhint("Count is '%ld'.", s_txn_reference_count)));
    }

    return s_txn_reference_count > 0;
}

bool adapter_t::begin_transaction()
{
    elog(DEBUG1, "Opening COW-SE transaction...");

    if (s_txn_reference_count < 0)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("A negative transaction count was detected in begin_transaction())!"),
             errhint("Count is '%ld'.", s_txn_reference_count)));
    }

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

    if (s_txn_reference_count <= 0)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("A non-positive transaction count was detected in commit_transaction())!"),
             errhint("Count is '%ld'.", s_txn_reference_count)));
    }

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

List* adapter_t::get_ddl_command_list(const char* server_name)
{
    try
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
    catch (const exception& e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Failed generating foreign table DDL strings."),
             errhint("Exception: %s.", e.what())));
    }
}

bool adapter_t::get_ids(
    const char* table_name,
    gaia::common::gaia_id_t& table_id,
    gaia::common::gaia_type_t& container_id)
{
    auto iterator = adapter_t::s_map_table_name_to_ids.find(table_name);
    if (iterator == adapter_t::s_map_table_name_to_ids.end())
    {
        return false;
    }

    table_id = iterator->second.first;
    container_id = iterator->second.second;

    return true;
}

bool state_t::initialize(const char* table_name, size_t count_fields)
{
    try
    {
        m_gaia_id_field_index = c_invalid_field_index;

        // Lookup table in our map.
        if (!adapter_t::get_ids(table_name, m_table_id, m_container_id))
        {
            return false;
        }

        // We start from 1 to cover gaia_id, which is not returned by list_fields.
        m_count_fields = 1;
        for (auto field_view : catalog_core_t::list_fields(m_table_id))
        {
            // We do not count anonymous references.
            if (field_view.data_type() == data_type_t::e_references
                && strlen(field_view.name()) == 0)
            {
                continue;
            }

            m_count_fields++;
        }

        if (count_fields != m_count_fields)
        {
            ereport(
                ERROR,
                (errcode(ERRCODE_FDW_ERROR),
                 errmsg("Unexpected table field count!"),
                 errhint("Count is '%ld' and expected count was '%ld'.", count_fields, m_count_fields)));
        }
        else
        {
            elog(LOG, "Successfully initialized processing of table %s with %ld fields!", table_name, count_fields);
        }

        // Allocate memory for holding field information.
        m_fields = reinterpret_cast<field_information_t*>(palloc0(
            sizeof(field_information_t) * m_count_fields));

        return true;
    }
    catch (const exception& e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Failed initializing FDW state."),
             errhint(
                 "Table: %s, container_id: %ld, field count: %ld. Exception: %s.",
                 table_name,
                 m_container_id,
                 count_fields,
                 e.what())));
    }
}

bool state_t::set_field_index(const char* field_name, size_t field_index)
{
    if (field_index >= m_count_fields)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Unexpected field index received in set_field_index()!"),
             errhint("Field index is '%ld' and field count is '%ld'.", field_index, m_count_fields)));
    }

    bool found_field = false;
    for (auto field_view : catalog_core_t::list_fields(m_table_id))
    {
        if (strcmp(field_name, field_view.name()) == 0)
        {
            found_field = true;

            m_fields[field_index].position = field_view.position();
            m_fields[field_index].type = field_view.data_type();

            if (adapter_t::is_gaia_id_name(field_name))
            {
                m_gaia_id_field_index = field_index;
            }

            break;
        }
    }

    return found_field;
}

bool state_t::is_gaia_id_field_index(size_t field_index)
{
    return field_index == m_gaia_id_field_index;
}

bool scan_state_t::initialize_scan()
{
    m_current_node = gaia_ptr::find_first(m_container_id);

    if (m_current_node)
    {
        m_current_payload = reinterpret_cast<uint8_t*>(m_current_node.data());
    }

    return true;
}

bool scan_state_t::has_scan_ended()
{
    return !m_current_node;
}

Datum scan_state_t::extract_field_value(size_t field_index)
{
    if (field_index >= m_count_fields)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Unexpected field index received in extract_field_value()!"),
             errhint("Field index is '%ld' and field count is '%ld'.", field_index, m_count_fields)));
    }

    try
    {
        Datum field_value{};

        if (is_gaia_id_field_index(field_index))
        {
            field_value = UInt64GetDatum(m_current_node.id());
        }
        else if (m_fields[field_index].type == data_type_t::e_references)
        {
            // TODO: handle references.
            field_value = UInt64GetDatum(0);
        }
        else
        {
            data_holder_t value = get_field_value(
                m_container_id,
                m_current_payload,
                nullptr,
                0,
                m_fields[field_index].position);

            field_value = convert_to_datum(value);
        }

        return field_value;
    }
    catch (const exception& e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Failed reading field value."),
             errhint(
                 "Container_id: %ld, field_index: %ld. Exception: %s.",
                 m_container_id,
                 field_index,
                 e.what())));
    }
}

bool scan_state_t::scan_forward()
{
    if (has_scan_ended())
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Attempt to scan forward after scan has ended!")));
    }

    m_current_node = m_current_node.find_next();
    m_current_payload = reinterpret_cast<uint8_t*>(m_current_node.data());

    return has_scan_ended();
}

void modify_state_t::copy_payload(const std::vector<uint8_t>& payload)
{
    m_current_payload_size = sizeof(uint8_t) * payload.size();
    m_current_payload = reinterpret_cast<uint8_t*>(palloc0(m_current_payload_size));
    memcpy(m_current_payload, payload.data(), m_current_payload_size);
}

bool modify_state_t::initialize(const char* table_name, size_t count_fields)
{
    if (!state_t::initialize(table_name, count_fields))
    {
        return false;
    }

    // Get hold of the type cache and lookup the type information for our type.
    auto_type_information_t auto_type_information;
    type_cache_t::get()->get_type_information(m_container_id, auto_type_information);

    // Set current payload to a copy of the serialization template bits.
    vector<uint8_t> current_payload = auto_type_information.get()->get_serialization_template();
    copy_payload(current_payload);

    // Get a pointer to the binary schema.
    m_binary_schema = auto_type_information.get()->get_raw_binary_schema();
    m_binary_schema_size = auto_type_information.get()->get_binary_schema_size();

    return true;
}

void modify_state_t::set_field_value(size_t field_index, const Datum& field_value)
{
    if (field_index >= m_count_fields)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Unexpected field index received in set_field_value()!"),
             errhint("Field index is '%ld' and field count is '%ld'.", field_index, m_count_fields)));
    }

    if (is_gaia_id_field_index(field_index))
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Attempt to set value of gaia_id field!")));
    }

    try
    {
        data_holder_t value = convert_to_data_holder(field_value, m_fields[field_index].type);

        if (value.type == reflection::String)
        {
            vector<uint8_t> updated_payload = ::set_field_value(
                m_container_id,
                m_current_payload,
                m_current_payload_size,
                m_binary_schema,
                m_binary_schema_size,
                m_fields[field_index].position,
                value);

            // Make a copy of the updated bits.
            copy_payload(updated_payload);
        }
        else
        {
            bool result = ::set_field_value(
                m_container_id,
                m_current_payload,
                m_binary_schema,
                m_binary_schema_size,
                m_fields[field_index].position,
                value);

            if (!result)
            {
                ereport(
                    ERROR,
                    (errcode(ERRCODE_FDW_ERROR),
                     errmsg("Failed to set field value!"),
                     errhint(
                         "Container id is '%ld' and field position is '%ld'.",
                         m_container_id,
                         m_fields[field_index].position)));
            }
        }
    }
    catch (const exception& e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Failed updating field value."),
             errhint(
                 "Container_id: %ld, field_index: %ld. Exception: %s.",
                 m_container_id,
                 field_index,
                 e.what())));
    }
}

bool modify_state_t::edit_record(uint64_t gaia_id, edit_state_t edit_state)
{
    bool result = false;
    try
    {
        if (edit_state == edit_state_t::create)
        {
            gaia_ptr::create(
                gaia_id,
                m_container_id,
                m_current_payload_size,
                m_current_payload);
        }
        else if (edit_state == edit_state_t::update)
        {
            auto node = gaia_ptr::open(gaia_id);
            node.update_payload(m_current_payload_size, m_current_payload);
        }

        result = true;
    }
    catch (const exception& e)
    {
        if (edit_state == edit_state_t::create)
        {
            ereport(
                ERROR,
                (errcode(ERRCODE_FDW_ERROR),
                 errmsg("Error creating gaia object."),
                 errhint("Exception: %s.", e.what())));
        }
        else if (edit_state == edit_state_t::update)
        {
            ereport(
                ERROR,
                (errcode(ERRCODE_FDW_ERROR),
                 errmsg("Error updating gaia object."),
                 errhint("Exception: %s.", e.what())));
        }

        return false;
    }

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
            ereport(
                ERROR,
                (errcode(ERRCODE_FDW_ERROR),
                 errmsg("Could not find record to delete."),
                 errhint("gaia_id: %ld.", gaia_id)));
            return false;
        }

        gaia_ptr::remove(node);

        return true;
    }
    catch (const exception& e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Error deleting gaia object."),
             errhint("Exception: %s.", e.what())));
    }

    return false;
}

} // namespace fdw
} // namespace gaia
