/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "scan_state.hpp"

#include <sstream>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/logger_internal.hpp"
#include "gaia_internal/db/catalog_core.hpp"

#include "field_access.hpp"
#include "reflection.hpp"

using namespace std;

using namespace gaia::catalog;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::payload_types;
using namespace nlohmann;

namespace gaia
{
namespace extract
{
#define Int16GetDatum(X) ((Datum)(X))
#define Int32GetDatum(X) ((Datum)(X))
#define Int64GetDatum(X) ((Datum)(X))
#define UInt64GetDatum(X) ((Datum)(X))
#define CStringGetDatum(X) ((Datum)(X))

static inline Datum
Float4GetDatum(float X)
{
    union
    {
        float value;
        int32_t retval;
    } myunion;

    myunion.value = X;
    return Int32GetDatum(myunion.retval);
}

static inline Datum
Float8GetDatum(double X)
{
    union
    {
        double value;
        int64_t retval;
    } myunion;

    myunion.value = X;
    return Int64GetDatum(myunion.retval);
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
        fprintf(stderr, "Unhandled data_holder_t type '%d'.\n", value.type);
    }
    return Datum{};
}

NullableDatum convert_to_nullable_datum(const data_holder_t& value)
{
    NullableDatum nullable_datum{};
    nullable_datum.value = 0;
    nullable_datum.isnull = false;

    if (value.type == reflection::String)
    {
        if (value.hold.string_value == nullptr)
        {
            nullable_datum.isnull = true;
            return nullable_datum;
        }

        // size_t string_length = strlen(value.hold.string_value);
        // if (string_length > 30) {
        //     fprintf(stderr, "A little worried here, string_length=%lu\n", string_length);
        // }
        // char* text = reinterpret_cast<char*>(malloc(string_length+1));

        // memcpy(text, value.hold.string_value, string_length);
        // text[string_length] = '\0';
        nullable_datum.value = CStringGetDatum(value.hold.string_value);
    }
    else
    {
        nullable_datum.value = convert_to_datum(value);
    }

    return nullable_datum;
}

bool scan_state_t::initialize_caches()
{
    for (auto table_view : catalog_core_t::list_tables())
    {
        string table_name(table_view.name());

        auto type_information = make_unique<type_information_t>();

        initialize_type_information_from_binary_schema(
            type_information.get(),
            table_view.binary_schema()->data(),
            table_view.binary_schema()->size());

        type_information.get()->set_serialization_template(
            table_view.serialization_template()->data(),
            table_view.serialization_template()->size());

        bool result = type_cache_t::get()->set_type_information(table_view.table_type(), type_information);
        // TODO: Verify that it's okay for the information to exist after the first initialization.
        // if (result == false)
        // {
        //     fprintf(stderr, "Failed to set type information in type_cache_t!\n");
        //     return false;
        // }
    }

    return true;
}

const char* scan_state_t::get_table_name()
{
    return m_table_name;
}

scan_state_t::scan_state_t()
{
    m_current_payload = nullptr;
}

bool scan_state_t::initialize_scan(gaia_type_t container_id, gaia_id_t start_after)
{
    if (!initialize_caches())
    {
        return false;
    }
    m_container_id = container_id;
    try
    {
        if (start_after != 0)
        {
            m_current_record = gaia_ptr_t::open(start_after);
            m_current_record = m_current_record.find_next();
            if (m_current_record.type() != container_id)
            {
                // This is expected when the scan is already at the end.
                // fprintf(stderr, "Starting row is not correct type.\n");
                return false;
            }
        }
        else
        {
            m_current_record = gaia_ptr_t::find_first(m_container_id);
        }

        if (m_current_record)
        {
            m_current_payload = reinterpret_cast<uint8_t*>(m_current_record.data());
        }

        return true;
    }
    catch (const exception& e)
    {
        fprintf(stderr, "Failed initializing table scan. "
                        "Table: '%s', container id: '%u'. Exception: '%s'.\n",
                get_table_name(), m_container_id, e.what());
    }
    return false;
}

bool scan_state_t::has_scan_ended()
{
    return !m_current_record;
}

NullableDatum scan_state_t::extract_field_value(uint16_t repeated_count, size_t position)
{
    try
    {
        NullableDatum field_value{};
        field_value.isnull = false;

        /* TODO: Remove postgres dependency in array handling.
        if (is_gaia_id_field_index(field_index))
        {
            field_value.value = UInt64GetDatum(m_current_record.id());
        }
        else if (m_fields[field_index].is_reference)
        {
            reference_offset_t reference_offset = m_fields[field_index].position;
            if (reference_offset >= m_current_record.num_references())
            {
                fprintf(stderr,"Attempt to dereference an invalid reference offset '%ld' for table '%s'!");
            }

            gaia_id_t reference_id = m_current_record.references()[reference_offset];
            field_value.value = UInt64GetDatum(reference_id);

            // If the reference id is invalid, surface the value as NULL.
            if (reference_id == c_invalid_gaia_id)
            {
                field_value.isnull = true;
            }
        }
        */
        if (repeated_count != 1)
        {
            size_t array_size = get_field_array_size(
                m_container_id,
                m_current_payload,
                nullptr,
                0,
                position);

            for (size_t i = 0; i < array_size; i++)
            {
                data_holder_t value = get_field_array_element(
                    m_container_id,
                    m_current_payload,
                    nullptr,
                    0,
                    position,
                    i);
            }
        }
        else
        {
            data_holder_t value = get_field_value(
                m_container_id,
                m_current_payload,
                nullptr,
                0,
                position);

            field_value = convert_to_nullable_datum(value);
        }

        return field_value;
    }
    catch (const exception& e)
    {
        fprintf(stderr, "Failed reading field value. "
                        "Table: '%s', container id: '%u', field index: '%ld'. Exception: '%s'.\n",
                get_table_name(), m_container_id, position, e.what());
    }

    return NullableDatum{};
}

bool scan_state_t::scan_forward()
{
    try
    {
        m_current_record = m_current_record.find_next();

        if (m_current_record)
        {
            m_current_payload = reinterpret_cast<uint8_t*>(m_current_record.data());
        }

        return has_scan_ended();
    }
    catch (const exception& e)
    {
        fprintf(stderr, "Failed iterating to next record. Table id: '%s', container id: '%u'. Exception: '%s'.\n", get_table_name(), m_container_id, e.what());
    }
    return false;
}

} // namespace extract
} // namespace gaia
