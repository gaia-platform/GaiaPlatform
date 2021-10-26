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

bool table_iterator_t::initialize_scan(gaia_type_t container_id, gaia_id_t start_after)
{
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
