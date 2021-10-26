/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <unordered_map>

#include "gaia/common.hpp"

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"

#include "field_access.hpp"
#include "json.hpp"

namespace gaia
{
namespace tools
{
namespace db_extract
{

using value_t = uintptr_t;

// A structure holding basic field information.
struct field_information_t
{
    // The position field can hold either a field_position_t or a reference_offset_t value,
    // depending on whether the field is a regular field or a reference field,
    // as indicated by the value of the is_reference field.
    static_assert(sizeof(gaia::common::field_position_t) <= sizeof(uint16_t));
    static_assert(sizeof(gaia::common::reference_offset_t) <= sizeof(uint16_t));
    uint16_t position;

    gaia::common::data_type_t type;

    uint64_t repeated_count;

    bool is_reference;
};

class table_iterator_t
{
    friend class extractor_t;

public:
    table_iterator_t();

    // Do not allow copies to be made;
    // disable copy constructor and assignment operator.
    table_iterator_t(const table_iterator_t&) = delete;
    table_iterator_t& operator=(const table_iterator_t&) = delete;

    // Provides the index corresponding to each field.
    // This enables future calls to use index values.
    bool set_field_index(const char* field_name, size_t field_index);

    bool is_gaia_id_field_index(size_t field_index);

    const char* get_table_name();

    // Scan API.
    bool initialize_scan(gaia::common::gaia_type_t, gaia::common::gaia_id_t);
    bool has_scan_ended();
    db::payload_types::data_holder_t extract_field_value(uint16_t, size_t field_index);
    value_t convert_to_value(const gaia::db::payload_types::data_holder_t& value);
    bool scan_forward();
    gaia::common::gaia_id_t gaia_id()
    {
        return m_current_record.id();
    }

private:
    bool initialize_caches();

private:
    // Store the table name for the convenience of printing it in error messages.
    char* m_table_name;

    // The table (container) id.
    gaia::common::gaia_type_t m_container_id;

    // Field information array.
    field_information_t* m_fields;

    // The COW-SE smart ptr we are currently iterating over.
    gaia::db::gaia_ptr_t m_current_record;

    // Pointer to the deserialized payload of the current record.
    const uint8_t* m_current_payload;
};

} // namespace db_extract
} // namespace tools
} // namespace gaia
