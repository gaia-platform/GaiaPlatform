/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <unordered_map>

#include <json.hpp>

#include "gaia/common.hpp"

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"

#include "field_access.hpp"

namespace gaia
{
namespace tools
{
namespace db_extract
{

class table_iterator_t
{
    friend class extractor_t;

public:
    table_iterator_t()
        : m_table_name(nullptr)
        , m_container_id(0)
        , m_current_record(db::gaia_ptr_t::from_locator(db::c_invalid_gaia_locator))
        , m_current_payload(nullptr)
    {
    }

    // Do not allow copies to be made;
    // disable copy constructor and assignment operator.
    table_iterator_t(const table_iterator_t&) = delete;
    table_iterator_t& operator=(const table_iterator_t&) = delete;

    // Scan API.
    bool set(gaia::common::gaia_type_t, gaia::common::gaia_id_t);
    bool initialize_scan(gaia::common::gaia_id_t, gaia::common::gaia_type_t, gaia::common::gaia_id_t);
    bool scan_forward();
    bool has_scan_ended()
    {
        return !m_current_record;
    }

    // Obtain contents.
    db::payload_types::data_holder_t extract_field_value(uint16_t, size_t field_index);

    const char* get_table_name()
    {
        return m_table_name;
    }

    gaia::common::gaia_id_t gaia_id()
    {
        return m_current_record.id();
    }

private:
    // Store the table name for the convenience of printing it in error messages.
    char* m_table_name;

    // The table id.
    gaia::common::gaia_id_t m_table_id;

    // The container (type) id.
    gaia::common::gaia_type_t m_container_id;

    // The COW-SE smart ptr we are currently iterating over.
    gaia::db::gaia_ptr_t m_current_record;

    // Pointer to the deserialized payload of the current record.
    const uint8_t* m_current_payload;
};

} // namespace db_extract
} // namespace tools
} // namespace gaia
