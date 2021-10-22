/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

// We use `#pragma GCC` instead of `#pragma clang` for compatibility with both clang and gcc.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmacro-redefined"
#define fprintf fprintf
#pragma GCC diagnostic pop

#include <string>
#include <unordered_map>

#include "gaia/common.hpp"

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"

#include "json.hpp"

namespace gaia
{
namespace tools
{
namespace db_extract
{

using datum_t = uintptr_t;

typedef struct nullable_datum_t
{
    datum_t value;
    bool isnull;
} nullable_datum_t;

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

    // Note: currently, this is used only for the delayed setting of references.
    nullable_datum_t value_to_set;
};

// The scan state is set up in gaia_begin_foreign_scan,
// is stashed away in node->fdw_private,
// and is fetched in gaia_iterate_foreign_scan and gaia_rescan_foreign_scan.
class scan_state_t
{
    friend class extractor_t;

public:
    scan_state_t();

    // Provides the index corresponding to each field.
    // This enables future calls to use index values.
    bool set_field_index(const char* field_name, size_t field_index);

    bool is_gaia_id_field_index(size_t field_index);

    const char* get_table_name();

    // Scan API.
    bool initialize_scan(gaia::common::gaia_type_t, gaia::common::gaia_id_t);
    bool has_scan_ended();
    nullable_datum_t extract_field_value(uint16_t, size_t field_index);
    bool scan_forward();
    gaia::common::gaia_id_t gaia_id()
    {
        return m_current_record.id();
    }

private:
    // Do not allow copies to be made;
    // disable copy constructor and assignment operator.
    scan_state_t(const scan_state_t&) = delete;
    scan_state_t& operator=(const scan_state_t&) = delete;

    bool initialize_caches();

    // Store the table name for the convenience of printing it in error messages.
    char* m_table_name;

    // The table id and container id.
    gaia::common::gaia_id_t m_table_id;
    gaia::common::gaia_type_t m_container_id;

    // Field information array.
    field_information_t* m_fields;

    // The COW-SE smart ptr we are currently iterating over.
    gaia::db::gaia_ptr_t m_current_record;

    // Pointer to the deserialized payload of the current record.
    const uint8_t* m_current_payload;

    // Small cache to enable looking up a table type by name.
    static std::unordered_map<std::string, std::pair<gaia::common::gaia_id_t, gaia::common::gaia_type_t>>
        s_map_table_name_to_ids;
};

} // namespace db_extract
} // namespace tools
} // namespace gaia