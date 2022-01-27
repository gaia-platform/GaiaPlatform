/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_fdw_adapter.hpp"

#include <sstream>

#include "gaia/common.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/logger.hpp"
#include "gaia_internal/db/catalog_core.hpp"

#include "field_access.hpp"
#include "reflection.hpp"

using namespace std;

using namespace gaia::catalog;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::payload_types;

// The gaia_fdw_adapter classes are meant to isolate the FDW from the internals of the Gaia database.
// Thus, all important FDW-database interactions go through adapter interfaces.
// Because Postgres does not handle exceptions gracefully, the adapter interfaces should not throw any
// and should instead use Postgres's error reporting methods.
// All database operations should be enclosed in a catch on the calling path from the FDW. Failure to do so
// represents an internal programming error.

namespace gaia
{
namespace fdw
{

static constexpr size_t c_invalid_field_index = std::numeric_limits<size_t>::max();

// NOLINTNEXTLINE(readability-identifier-naming)
static constexpr char c_message_fdw_internal_error[] = "An FDW internal error was detected in '%s'!";
// NOLINTNEXTLINE(readability-identifier-naming)
static constexpr char c_message_exception[] = "Exception: '%s'.";

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

// Make sure this function matches the imported FDW schema.
// The type conversion is defined in get_fdw_data_type_name().
Oid convert_to_pg_type(data_type_t type)
{
    switch (type)
    {
    case data_type_t::e_bool:
        return BOOLOID;

    case data_type_t::e_int8:
    case data_type_t::e_uint8:
    case data_type_t::e_int16:
    case data_type_t::e_uint16:
        return INT2OID;

    case data_type_t::e_int32:
    case data_type_t::e_uint32:
        return INT4OID;

    case data_type_t::e_int64:
    case data_type_t::e_uint64:
        return INT8OID;

    case data_type_t::e_float:
        return FLOAT4OID;

    case data_type_t::e_double:
        return FLOAT8OID;

    default:
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg(c_message_fdw_internal_error, __func__),
             errhint("Unhandled data type '%d'.", type)));
    }
}

// Convert a non-string data holder value to PostgreSQL Datum.
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

    default:
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg(c_message_fdw_internal_error, __func__),
             errhint("Unhandled data_holder_t type '%d'.", value.type)));
    }
}

NullableDatum convert_to_nullable_datum(const data_holder_t& value)
{
    NullableDatum nullable_datum{};
    nullable_datum.value = 0;
    nullable_datum.isnull = value.is_null;

    if (value.is_null)
    {
        return nullable_datum;
    }

    if (value.type == reflection::String)
    {
        size_t string_length = strlen(value.hold.string_value);
        size_t pg_text_length = string_length + VARHDRSZ;
        text* pg_text = reinterpret_cast<text*>(palloc(pg_text_length));

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
        SET_VARSIZE(pg_text, pg_text_length);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
        memcpy(VARDATA(pg_text), value.hold.string_value, string_length);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
        nullable_datum.value = CStringGetDatum(pg_text);
    }
    else
    {
        nullable_datum.value = convert_to_datum(value);
    }

    return nullable_datum;
}

data_holder_t convert_to_data_holder(const Datum& value, data_type_t value_type)
{
    data_holder_t data_holder;
    data_holder.is_null = false;

    try
    {
        data_holder.type = convert_to_reflection_type(value_type);
    }
    catch (const exception& e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg(c_message_fdw_internal_error, __func__),
             errhint("Unhandled data_type_t '%d'.", value_type)));
    }

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
        data_holder.hold.integer_value = DatumGetInt64(value);
        break;

    case data_type_t::e_float:
        data_holder.hold.float_value = DatumGetFloat4(value);
        break;

    case data_type_t::e_double:
        data_holder.hold.float_value = DatumGetFloat8(value);
        break;

    case data_type_t::e_string:
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
        data_holder.hold.string_value = TextDatumGetCString(value);
        break;

    default:
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg(c_message_fdw_internal_error, __func__),
             errhint("Unhandled data_type_t '%d'.", value_type)));
    }

    return data_holder;
}

void adapter_t::initialize_caches()
{
    gaia::db::begin_transaction();

    ASSERT_PRECONDITION(
        type_cache_t::get()->size() == 0,
        "type_cache_t has been initialized already!");
    ASSERT_PRECONDITION(
        s_map_table_name_to_ids.size() == 0,
        "s_map_table_name_to_ids has been initialized already!");

    for (auto table_view : catalog_core::list_tables())
    {
        elog(
            DEBUG1, "Loading metadata information for table `%s' with type '%ld' and id '%ld'...",
            table_view.name(), table_view.table_type(), table_view.id());

        string table_name(table_view.name());

        if (table_view.binary_schema()->size() == 0)
        {
            elog(ERROR, "Table '%s' is missing binary schema data!", table_view.name());
        }

        if (table_view.serialization_template()->size() == 0)
        {
            elog(ERROR, "Table '%s' is missing serialization template data!", table_view.name());
        }

        auto type_information = make_unique<type_information_t>();

        initialize_type_information_from_binary_schema(
            type_information.get(),
            table_view.binary_schema()->data(),
            table_view.binary_schema()->size());

        type_information.get()->set_serialization_template(
            table_view.serialization_template()->data(),
            table_view.serialization_template()->size());

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
                 "type_cache_t has size '%ld', but s_map_table_name_to_ids has size '%ld'.",
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
        gaia_log::initialize({});

        gaia::db::begin_session();

        initialize_caches();
    }
    catch (const exception& e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Error opening COW-SE session."),
             errhint(c_message_exception, e.what())));
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
             errhint(c_message_exception, e.what())));
    }
}

bool adapter_t::begin_transaction()
{
    elog(DEBUG1, "Opening COW-SE transaction...");

    try
    {
        ASSERT_PRECONDITION(
            s_txn_reference_count >= 0,
            "A negative transaction count was detected in begin_transaction())!");

        bool opened_transaction = false;
        int previous_count = s_txn_reference_count++;
        if (previous_count == 0)
        {
            opened_transaction = true;
            gaia::db::begin_transaction();
        }

        elog(DEBUG1, "Txn actually opened: '%s'.", opened_transaction ? "true" : "false");

        return opened_transaction;
    }
    catch (const exception& e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Error beginning transaction."),
             errhint(c_message_exception, e.what())));
    }
}

bool adapter_t::commit_transaction()
{
    elog(DEBUG1, "Closing COW-SE transaction...");

    try
    {
        ASSERT_PRECONDITION(
            s_txn_reference_count > 0,
            "A non-positive transaction count was detected in commit_transaction())!");

        bool closed_transaction = false;
        int previous_count = s_txn_reference_count--;
        if (previous_count == 1)
        {
            closed_transaction = true;
            gaia::db::commit_transaction();
        }

        elog(DEBUG1, "Txn actually closed: '%s'.", closed_transaction ? "true" : "false");

        return closed_transaction;
    }
    catch (const exception& e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Error committing transaction."),
             errhint(c_message_exception, e.what())));
    }
}

bool adapter_t::is_gaia_id_name(const char* name)
{
    constexpr char c_gaia_id[] = "gaia_id";

    return strcmp(c_gaia_id, name) == 0;
}

uint64_t adapter_t::get_new_gaia_id()
{
    try
    {
        return gaia_ptr_t::generate_id();
    }
    catch (const exception& e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Error generating gaia_id."),
             errhint(c_message_exception, e.what())));
    }
}

List* adapter_t::get_ddl_command_list(const char* server_name)
{
    try
    {
        List* commands = NIL;

        gaia::db::begin_transaction();

        for (auto table_view : catalog_core::list_tables())
        {
            // Generate DDL statement for current table and log it.
            string ddl_formatted_statement = gaia::catalog::generate_fdw_ddl(table_view.id(), server_name);
            elog(DEBUG1, ddl_formatted_statement.c_str());

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
             errhint(c_message_exception, e.what())));
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

bool state_t::initialize(const char* table_name, size_t expected_field_count)
{
    try
    {
        m_gaia_id_field_index = c_invalid_field_index;

        // Lookup table in our map.
        if (!adapter_t::get_ids(table_name, m_table_id, m_container_id))
        {
            return false;
        }

        // Our field count consists of all fields and references, plus the gaia_id system field.
        size_t field_count = list_fields(m_table_id).size();
        size_t reference_count = list_references(m_table_id).size();
        m_field_count = 1 + field_count + reference_count;
        if (m_field_count != expected_field_count)
        {
            ereport(
                ERROR,
                (errcode(ERRCODE_FDW_ERROR),
                 errmsg("Unexpected table field count!"),
                 errhint(
                     "Field count is '%ld' and expected count was '%ld' for table '%s'.",
                     m_field_count, expected_field_count, table_name)));
        }
        else
        {
            elog(
                DEBUG1, "Successfully initialized processing of table '%s' with '%ld' fields!",
                table_name, m_field_count);
        }

        // Allocate memory for holding field information.
        size_t fields_array_size = sizeof(field_information_t) * m_field_count;
        m_fields = reinterpret_cast<field_information_t*>(palloc0(fields_array_size));

        // Allocate memory for holding the table name.
        size_t table_name_size = strlen(table_name) + 1;
        m_table_name = reinterpret_cast<char*>(palloc0(table_name_size));
        strlcpy(m_table_name, table_name, table_name_size);

        return true;
    }
    catch (const exception& e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Failed initializing FDW state."),
             errhint(
                 "Table: '%s', container id: '%ld', expected field count: '%ld'. Exception: '%s'.",
                 table_name, m_container_id, expected_field_count, e.what())));
    }
}

bool state_t::set_field_index(const char* field_name, size_t field_index)
{
    if (field_index >= m_field_count)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Unexpected field index received in set_field_index()!"),
             errhint(
                 "Field index is '%ld' and field count is '%ld' for table '%s'.",
                 field_index, m_field_count, get_table_name())));
    }

    try
    {
        // gaia_id is an implicit field that is not described in metadata,
        // so it needs special handling.
        if (adapter_t::is_gaia_id_name(field_name))
        {
            m_gaia_id_field_index = field_index;

            m_fields[field_index].position = c_invalid_field_position;
            m_fields[field_index].type = data_type_t::e_uint64;
            m_fields[field_index].is_reference = false;
            m_fields[field_index].repeated_count = 1;

            return true;
        }

        // All other fields need to be looked up in metadata.
        bool found_field = false;

        // Look up fields first.
        for (gaia_id_t field_id : gaia::catalog::list_fields(m_table_id))
        {
            gaia_field_t field = gaia_field_t::get(field_id);

            if (strcmp(field_name, field.name()) == 0)
            {
                found_field = true;

                m_fields[field_index].position = field.position();
                m_fields[field_index].type = static_cast<data_type_t>(field.type());
                m_fields[field_index].is_reference = false;
                m_fields[field_index].repeated_count = field.repeated_count();

                break;
            }
        }

        // Lookup references if we have not found a match in fields.
        for (gaia_id_t reference_id : list_references(m_table_id))
        {
            gaia_relationship_t relationship = gaia_relationship_t::get(reference_id);

            // Only the to_parent link is relevant because that's how SQL works:
            // you have a foreign key pointing to the parent in the child table.
            string relationship_name = relationship.to_parent_link_name();

            if (strcmp(field_name, relationship_name.c_str()) == 0)
            {
                found_field = true;

                m_fields[field_index].position = relationship.parent_offset();
                m_fields[field_index].type = data_type_t::e_uint64;
                m_fields[field_index].is_reference = true;
                m_fields[field_index].repeated_count = 1;

                break;
            }
        }

        return found_field;
    }
    catch (const exception& e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Failed setting field index."),
             errhint(
                 "Table: '%s', container id: '%ld', field: '%s', field index: '%ld'. Exception: '%s'.",
                 get_table_name(), m_container_id, field_name, field_index, e.what())));
    }
}

bool state_t::is_gaia_id_field_index(size_t field_index)
{
    return field_index == m_gaia_id_field_index;
}

const char* state_t::get_table_name()
{
    return m_table_name;
}

scan_state_t::scan_state_t()
{
    m_current_payload = nullptr;
}

bool scan_state_t::initialize_scan()
{
    try
    {
        m_current_record = gaia_ptr_t::find_first(m_container_id);

        if (m_current_record)
        {
            m_current_payload = reinterpret_cast<uint8_t*>(m_current_record.data());
        }

        return true;
    }
    catch (const exception& e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Failed initializing table scan."),
             errhint(
                 "Table: '%s', container id: '%ld'. Exception: '%s'.",
                 get_table_name(), m_container_id, e.what())));
    }
}

bool scan_state_t::has_scan_ended()
{
    return !m_current_record;
}

NullableDatum scan_state_t::extract_field_value(size_t field_index)
{
    if (field_index >= m_field_count)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Unexpected field index received in %s!", __func__),
             errhint(
                 "Field index is '%ld' and field count is '%ld' for table '%s'.",
                 field_index, m_field_count, get_table_name())));
    }

    try
    {
        NullableDatum field_value{};
        field_value.isnull = false;

        if (is_gaia_id_field_index(field_index))
        {
            field_value.value = UInt64GetDatum(m_current_record.id());
        }
        else if (m_fields[field_index].is_reference)
        {
            reference_offset_t reference_offset = m_fields[field_index].position;
            if (reference_offset >= m_current_record.references_count())
            {
                ereport(
                    ERROR,
                    (errcode(ERRCODE_FDW_ERROR),
                     errmsg(
                         "Attempt to dereference an invalid reference offset '%ld' for table '%s'!",
                         reference_offset, get_table_name())));
            }

            gaia_id_t anchor_id = m_current_record.references()[reference_offset];
            if (anchor_id == c_invalid_gaia_id)
            {
                field_value.value = UInt64GetDatum(c_invalid_gaia_id);
                field_value.isnull = true;
            }
            else
            {
                gaia_id_t reference_id = gaia_ptr_t::from_gaia_id(anchor_id).references()[c_ref_anchor_parent_offset];
                field_value.value = UInt64GetDatum(reference_id);

                // If the reference id is invalid, surface the value as NULL.
                if (reference_id == c_invalid_gaia_id)
                {
                    field_value.isnull = true;
                }
            }
        }
        else if (m_fields[field_index].repeated_count != 1)
        {
            Oid element_type = convert_to_pg_type(m_fields[field_index].type);

            size_t array_size = get_field_array_size(
                m_container_id,
                m_current_payload,
                nullptr,
                0,
                m_fields[field_index].position);

            if (array_size == std::numeric_limits<size_t>::max())
            {
                field_value.isnull = true;
            }
            else
            {
                ArrayBuildState* state = initArrayResult(element_type, CurrentMemoryContext, true);

                for (size_t i = 0; i < array_size; i++)
                {
                    data_holder_t value = get_field_array_element(
                        m_container_id,
                        m_current_payload,
                        nullptr,
                        0,
                        m_fields[field_index].position,
                        i);

                    accumArrayResult(
                        state,
                        convert_to_datum(value),
                        false,
                        element_type,
                        CurrentMemoryContext);
                }

                field_value.value = makeArrayResult(state, CurrentMemoryContext);
            }
        }
        else
        {
            data_holder_t value = get_field_value(
                m_container_id,
                m_current_payload,
                nullptr,
                0,
                m_fields[field_index].position);

            field_value = convert_to_nullable_datum(value);
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
                 "Table: '%s', container id: '%ld', field index: '%ld'. Exception: '%s'.",
                 get_table_name(), m_container_id, field_index, e.what())));
    }
}

bool scan_state_t::scan_forward()
{
    try
    {
        ASSERT_PRECONDITION(!has_scan_ended(), "Attempt to scan forward after scan has ended!");

        m_current_record = m_current_record.find_next();

        if (m_current_record)
        {
            m_current_payload = reinterpret_cast<uint8_t*>(m_current_record.data());
        }

        return has_scan_ended();
    }
    catch (const exception& e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Failed iterating to next record."),
             errhint(
                 "Table id: '%s', container id: '%ld'. Exception: '%s'.",
                 get_table_name(), m_container_id, e.what())));
    }
}

modify_state_t::modify_state_t()
{
    m_current_payload = nullptr;
    m_binary_schema = nullptr;
    m_binary_schema_size = 0;
}

void modify_state_t::initialize_payload()
{
    try
    {
        // Get hold of the type cache and lookup the type information for our type.
        auto_type_information_t auto_type_information;
        type_cache_t::get()->get_type_information(m_container_id, auto_type_information);

        // Set current payload to a copy of the serialization template bits.
        m_current_payload = new vector<uint8_t>(
            auto_type_information.get()->get_serialization_template(),
            auto_type_information.get()->get_serialization_template()
                + auto_type_information.get()->get_serialization_template_size());

        // Get a pointer to the binary schema.
        // We only need to do this on the first call.
        if (m_binary_schema == nullptr)
        {
            m_binary_schema = auto_type_information.get()->get_binary_schema();
            m_binary_schema_size = auto_type_information.get()->get_binary_schema_size();
        }
    }
    catch (const exception& e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Failed to initialize record payload."),
             errhint(
                 "Table id: '%s', container id: '%ld'. Exception: '%s'.",
                 get_table_name(), m_container_id, e.what())));
    }
}

void modify_state_t::set_field_value(size_t field_index, const NullableDatum& field_value)
{
    if (field_index >= m_field_count)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Unexpected field index received in '%s'!", __func__),
             errhint(
                 "Field index is '%ld' and field count is '%ld' for table '%s'.",
                 field_index, m_field_count, get_table_name())));
    }

    if (is_gaia_id_field_index(field_index))
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Attempt to set the value of the gaia_id field!")));
    }

    if (field_value.isnull && !m_fields[field_index].is_reference)
    {
        // For now, in the case of regular field updates,
        // the reflection API does not support setting nullptr values,
        // so we'll process NULL by doing nothing,
        // which will result in fields getting set to default values.
        return;
    }

    if (m_fields[field_index].is_reference)
    {
        // Reference fields are stored separately from regular fields and are set directly in the record.
        // At this point, we may not even have a record to update yet (we may be called for an insert),
        // so we'll just remember the edit request in the fields array
        // and we'll delay performing the update until modify_record() gets called.
        m_fields[field_index].value_to_set = field_value;
        return;
    }

    try
    {
        if (m_fields[field_index].repeated_count == 1)
        {
            // Regular fields are serialized in the payload of the current record.
            data_holder_t value = convert_to_data_holder(field_value.value, m_fields[field_index].type);

            if (value.type == reflection::String)
            {
                ::set_field_value(
                    m_container_id,
                    *m_current_payload,
                    m_binary_schema,
                    m_binary_schema_size,
                    m_fields[field_index].position,
                    value);
            }
            else
            {
                bool result = ::set_field_value(
                    m_container_id,
                    m_current_payload->data(),
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
                             "Table is '%s', container id is '%ld', and field position is '%ld'.",
                             get_table_name(), m_container_id, m_fields[field_index].position)));
                }
            }
        }
        else
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
            ArrayType* pg_array = DatumGetArrayTypeP(field_value.value);

            // PostgreSQL supports setting array element values to NULL even for
            // scalar types. It will be difficult for us to implement the
            // feature because both std::vector and flatbuffers::vector do not
            // support it. Do not allow this behavior at the moment.
            if (array_contains_nulls(pg_array))
            {
                ereport(
                    ERROR,
                    (errcode(ERRCODE_FDW_ERROR),
                     errmsg("Failed to set field value!"),
                     errhint(
                         "Setting array element value to NULL is not supported.",
                         get_table_name(), m_container_id, m_fields[field_index].position)));
            }

            Oid element_type = ARR_ELEMTYPE(pg_array);

            // The followings are array element type metadata that are needed to
            // access array elements. Their definitions can be found in the
            // PostgreSQL system catalog pg_type.
            int16 element_typlen;
            bool element_typbyval;
            char element_typalign;
            get_typlenbyvalalign(element_type, &element_typlen, &element_typbyval, &element_typalign);

            // Use deconstruct_array() to retrieve element values from the array.
            int num_elements;
            Datum* values;
            bool* nulls;
            deconstruct_array(
                pg_array,
                element_type,
                element_typlen,
                element_typbyval,
                element_typalign,
                &values,
                &nulls,
                &num_elements);

            ::set_field_array_size(
                m_container_id,
                *m_current_payload,
                m_binary_schema,
                m_binary_schema_size,
                m_fields[field_index].position,
                num_elements);

            for (int array_index = 0; array_index < num_elements; array_index++)
            {
                data_holder_t element_value
                    = convert_to_data_holder(values[array_index], m_fields[field_index].type);

                ::set_field_array_element(
                    m_container_id,
                    m_current_payload->data(),
                    m_binary_schema,
                    m_binary_schema_size,
                    m_fields[field_index].position,
                    array_index,
                    element_value);
            }
        }
    }
    catch (const exception& e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("Failed to set field value!"),
             errhint(
                 "Table: '%s', container id: '%ld', field index: '%ld'. Exception: '%s'.",
                 get_table_name(), m_container_id, field_index, e.what())));
    }
}

bool modify_state_t::modify_record(uint64_t gaia_id, modify_operation_type_t modify_operation_type)
{
    try
    {
        gaia_ptr_t record;
        if (modify_operation_type == modify_operation_type_t::insert)
        {
            record = gaia_ptr::create(gaia_id, m_container_id, m_current_payload->size(), m_current_payload->data());
        }
        else if (modify_operation_type == modify_operation_type_t::update)
        {
            record = gaia_ptr_t::from_gaia_id(gaia_id);

            // Only update payload if it has changed.
            if (record.data_size() != m_current_payload->size()
                || memcmp(record.data(), m_current_payload->data(), record.data_size()) != 0)
            {
                gaia_ptr::update_payload(record, m_current_payload->size(), m_current_payload->data());
            }
        }
        else
        {
            ereport(
                ERROR,
                (errcode(ERRCODE_FDW_ERROR),
                 errmsg(
                     "'%s' was called with an invalid operation type '%d'!",
                     __func__, static_cast<int>(modify_operation_type))));
        }

        // Now that we have access to the database record, we can also perform the reference updates.
        for (size_t i = 0; i < m_field_count; i++)
        {
            if (!m_fields[i].is_reference)
            {
                continue;
            }

            reference_offset_t reference_offset = m_fields[i].position;
            if (reference_offset >= record.references_count())
            {
                ereport(
                    ERROR,
                    (errcode(ERRCODE_FDW_ERROR),
                     errmsg(
                         "Attempt to dereference an invalid reference offset '%d' for table '%s'!",
                         reference_offset, get_table_name())));
            }

            // Read the existing reference value.
            gaia_id_t old_reference_id = record.references()[reference_offset];

            if (m_fields[i].value_to_set.isnull)
            {
                // If inserting a new record or if the existing reference is not set,
                // we can just leave the reference unset.
                if (modify_operation_type == modify_operation_type_t::insert
                    || old_reference_id == c_invalid_gaia_id)
                {
                    continue;
                }

                // If the existing reference was valid, we need to remove it.
                gaia_ptr::remove_from_reference_container(gaia_id, reference_offset);
            }
            else
            {
                gaia_id_t new_reference_id = DatumGetUInt64(m_fields[i].value_to_set.value);

                if (new_reference_id == c_invalid_gaia_id)
                {
                    ereport(
                        ERROR,
                        (errcode(ERRCODE_FDW_ERROR),
                         errmsg(
                             "An invalid reference value was passed as a non-null value '%ld' for table '%s'!",
                             new_reference_id, get_table_name())));
                }

                // If updating an existing record and the reference value is unchanged,
                // we don't have to do anything.
                if (modify_operation_type == modify_operation_type_t::update
                    && new_reference_id == old_reference_id)
                {
                    continue;
                }

                // Update the reference to the new value.
                gaia_ptr::update_parent_reference(record, new_reference_id, reference_offset);
            }
        }

        return true;
    }
    catch (const exception& e)
    {
        constexpr char c_operation_insert[] = "INSERT";
        constexpr char c_operation_update[] = "UPDATE";
        constexpr char c_operation_unexpected[] = "Unexpected";

        const char* operation_name;

        if (modify_operation_type == modify_operation_type_t::insert)
        {
            operation_name = c_operation_insert;
        }
        else if (modify_operation_type == modify_operation_type_t::update)
        {
            operation_name = c_operation_update;
        }
        else
        {
            operation_name = c_operation_unexpected;
        }

        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("%s operation failed!", operation_name),
             errhint(
                 "Table: '%s', container id: '%ld'. Exception: '%s'.",
                 get_table_name(), m_container_id, e.what())));
    }
}

bool modify_state_t::insert_record(uint64_t gaia_id)
{
    return modify_record(gaia_id, modify_operation_type_t::insert);
}

bool modify_state_t::update_record(uint64_t gaia_id)
{
    return modify_record(gaia_id, modify_operation_type_t::update);
}

bool modify_state_t::delete_record(uint64_t gaia_id)
{
    try
    {
        auto record = gaia_ptr_t::from_gaia_id(gaia_id);
        if (!record)
        {
            ereport(
                ERROR,
                (errcode(ERRCODE_FDW_ERROR),
                 errmsg("Could not find record to delete."),
                 errhint(
                     "gaia_id '%ld', table '%s', container id '%ld'.",
                     gaia_id, get_table_name(), m_container_id)));
            return false;
        }

        gaia_ptr::remove(record);

        return true;
    }
    catch (const exception& e)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_FDW_ERROR),
             errmsg("DELETE operation failed!"),
             errhint(
                 "Table: '%s', container id: '%ld'. Exception: '%s'.",
                 get_table_name(), m_container_id, e.what())));
    }
}

void modify_state_t::end_modify()
{
    if (m_current_payload)
    {
        delete m_current_payload;
        m_current_payload = nullptr;
    }
}

} // namespace fdw
} // namespace gaia
