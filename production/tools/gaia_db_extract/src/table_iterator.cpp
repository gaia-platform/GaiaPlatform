/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "table_iterator.hpp"

#include <sstream>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/logger_internal.hpp"
#include "gaia_internal/db/catalog_core.hpp"

#include "reflection.hpp"

using namespace std;

using namespace gaia::catalog;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::payload_types;
using namespace nlohmann;

namespace gaia
{
namespace tools
{
namespace db_extract
{

// The table_iterator_t object is used to support the interpretation of a single table's
// objects, obtaining the individual field values from each object (row). It reads through
// a single table type (gaia_type_t in the database) one object at a time, from the first to
// the last. The field values are obtained using flatbuffers reflection.
//
// The form of this object was borrowed and simplified from the SQL FDW adapter,
// in gaia_fdw_adapter.cpp, which also interfaces with Postgres. As this utility does
// not use Postgres, all of the Postgres-specific code has been eliminated.

static inline value_t
float_get_value(float X)
{
    union
    {
        float value;
        int32_t return_value;
    } float_union_t;

    float_union_t.value = X;
    return float_union_t.return_value;
}

static inline value_t
double_get_value(double X)
{
    union
    {
        double value;
        int64_t return_value;
    } double_union_t;

    double_union_t.value = X;
    return double_union_t.return_value;
}

// Convert a non-string data holder value to value_t.
value_t table_iterator_t::convert_to_value(const data_holder_t& value)
{
    switch (value.type)
    {
    case reflection::Bool:
    case reflection::Byte:
    case reflection::UByte:
    case reflection::Short:
    case reflection::UShort:
        return static_cast<int16_t>(value.hold.integer_value);

    case reflection::Int:
    case reflection::UInt:
        return static_cast<int32_t>(value.hold.integer_value);

    case reflection::Long:
    case reflection::ULong:
        return value.hold.integer_value;

    case reflection::Float:
        return float_get_value(static_cast<float>(value.hold.float_value));

    case reflection::Double:
        return double_get_value(value.hold.float_value);

    default:
        fprintf(stderr, "Unhandled data_holder_t type '%d'.\n", value.type);
    }
    return value_t{};
}

bool table_iterator_t::initialize_caches()
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

        // It's okay for the information to exist after the first initialization.
        type_cache_t::get()->set_type_information(table_view.table_type(), type_information);
    }

    return true;
}

const char* table_iterator_t::get_table_name()
{
    return m_table_name;
}

table_iterator_t::table_iterator_t()
{
    m_current_payload = nullptr;
}

bool table_iterator_t::initialize_scan(gaia_type_t container_id, gaia_id_t start_after)
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
            if (m_current_record.type() != container_id)
            {
                fprintf(stderr, "Starting row is not correct type.\n");
                return false;
            }
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
        fprintf(stderr, "Failed initializing table scan. Table: '%s', container id: '%u'. Exception: '%s'.\n", get_table_name(), m_container_id, e.what());
    }
    return false;
}

bool table_iterator_t::has_scan_ended()
{
    return !m_current_record;
}

data_holder_t table_iterator_t::extract_field_value(uint16_t repeated_count, size_t position)
{
    try
    {

        // TODO: Decide how arrays should be represented. Currently nothing will happen,
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
                auto value = get_field_array_element(
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
            return get_field_value(m_container_id, m_current_payload, nullptr, 0, position);
        }
    }
    catch (const exception& e)
    {
        fprintf(
            stderr,
            "Failed reading field value. Table: '%s', container id: '%u', field index: '%ld'. Exception: '%s'.\n",
            get_table_name(), m_container_id, position, e.what());
    }

    return data_holder_t{};
}

bool table_iterator_t::scan_forward()
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
        fprintf(
            stderr,
            "Failed iterating to next record. Table id: '%s', container id: '%u'. Exception: '%s'.\n",
            get_table_name(), m_container_id, e.what());
    }
    return false;
}

} // namespace db_extract
} // namespace tools
} // namespace gaia
