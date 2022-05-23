////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "table_iterator.hpp"

#include <sstream>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/logger.hpp"
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
bool table_iterator_t::prepare(gaia_id_t table_id, gaia_type_t container_id, gaia_id_t record_id)
{
    m_table_id = table_id;
    m_container_id = container_id;

    try
    {
        if (initialize_scan(container_id, record_id))
        {
            return true;
        }
    }
    catch (const exception& e)
    {
        fprintf(
            stderr,
            "Failed initializing table scan. Table: '%s', container id: '%u'. Exception: '%s'.\n",
            get_table_name(), m_container_id.value(), e.what());
    }
    return false;
}

// For the table_iterator_t::initialize_scan, the record_id parameter passed to
// table_iterator_t::prepare represents the row to start after.
bool table_iterator_t::initialize_scan(gaia_type_t container_id, gaia_id_t start_after)
{
    if (start_after.is_valid())
    {
        m_current_record = gaia_ptr_t::from_gaia_id(start_after);
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

std::vector<data_holder_t> table_iterator_t::extract_field_vector_value(size_t position)
{
    std::vector<data_holder_t> return_value;
    try
    {
        catalog_core::table_view_t table_view = catalog_core::get_table(m_table_id);
        auto schema = table_view.binary_schema();

        size_t array_size = get_field_array_size(
            m_current_payload,
            schema->data(),
            position);

        for (size_t i = 0; i < array_size; i++)
        {
            return_value.push_back(get_field_array_element(
                m_current_payload,
                schema->data(),
                position,
                i));
        }
    }
    catch (const exception& e)
    {
        fprintf(
            stderr,
            "Failed reading field value. Table: '%s', container id: '%u', field index: '%ld'. Exception: '%s'.\n",
            get_table_name(), m_container_id.value(), position, e.what());
    }

    return return_value;
}

data_holder_t table_iterator_t::extract_field_value(size_t position)
{
    try
    {
        catalog_core::table_view_t table_view = catalog_core::get_table(m_table_id);
        auto schema = table_view.binary_schema();
        return get_field_value(m_current_payload, schema->data(), position);
    }
    catch (const exception& e)
    {
        fprintf(
            stderr,
            "Failed reading field value. Table: '%s', container id: '%u', field index: '%ld'. Exception: '%s'.\n",
            get_table_name(), m_container_id.value(), position, e.what());
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
            get_table_name(), m_container_id.value(), e.what());
    }
    return false;
}

// For the related_table_iterator_t::initialize_scan, the record_id parameter passed to
// table_iterator_t::prepare represents the related record_id to fetch. For the related table,
// the "scan" is driven by a set of prefetched record ids.
bool related_table_iterator_t::initialize_scan(gaia_type_t container_id, gaia_id_t record_id)
{
    if (record_id.is_valid())
    {
        m_current_record = gaia_ptr_t::from_gaia_id(record_id);
        if (m_current_record.type() != container_id)
        {
            fprintf(stderr, "Row is not correct type.\n");
            return false;
        }
    }
    else
    {
        return false;
    }

    if (m_current_record)
    {
        m_current_payload = reinterpret_cast<uint8_t*>(m_current_record.data());
    }

    return true;
}

} // namespace db_extract
} // namespace tools
} // namespace gaia
