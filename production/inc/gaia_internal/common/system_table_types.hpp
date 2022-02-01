/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <limits>

#include "gaia/common.hpp"

namespace gaia
{
namespace common
{

constexpr gaia_type_t c_system_table_reserved_range = 4096;
constexpr gaia_type_t c_system_table_reserved_range_end = std::numeric_limits<gaia_type_t::value_type>::max();
constexpr gaia_type_t c_system_table_reserved_range_start
    = c_system_table_reserved_range_end.value() - c_system_table_reserved_range.value() + 1;

// NOTE: The following tables are essential for the db server to operate at
// lowest level. Consider adding to system tables instead unless necessary.
enum class catalog_core_table_type_t : gaia_type_t::value_type
{
    gaia_ref_anchor = c_system_table_reserved_range_end.value(),
    gaia_field = gaia_ref_anchor - 1,
    gaia_table = gaia_field - 1,
    gaia_database = gaia_table - 1,
    gaia_relationship = gaia_database - 1,
    gaia_index = gaia_relationship - 1,
    last = gaia_index
};

enum class system_table_type_t : gaia_type_t::value_type
{
    catalog_gaia_table = static_cast<gaia_type_t::value_type>(catalog_core_table_type_t::gaia_table),
    catalog_gaia_field = static_cast<gaia_type_t::value_type>(catalog_core_table_type_t::gaia_field),
    catalog_gaia_database = static_cast<gaia_type_t::value_type>(catalog_core_table_type_t::gaia_database),
    catalog_gaia_relationship = static_cast<gaia_type_t::value_type>(catalog_core_table_type_t::gaia_relationship),
    catalog_gaia_index = static_cast<gaia_type_t::value_type>(catalog_core_table_type_t::gaia_index),
    catalog_gaia_ref_anchor = static_cast<gaia_type_t::value_type>(catalog_core_table_type_t::gaia_ref_anchor),
    // Assign constant IDs to other system tables starting from lower end of the reserved range.
    event_log = c_system_table_reserved_range_start.value(),
    gaia_ruleset = event_log + 1,
    gaia_rule = gaia_ruleset + 1,
    gaia_application = gaia_rule + 1,
    app_database = gaia_application + 1,
    app_ruleset = app_database + 1,
    ruleset_database = app_ruleset + 1,
    rule_table = ruleset_database + 1,
    rule_field = rule_table + 1,
    rule_relationship = rule_field + 1,
};

inline bool is_catalog_core_object(gaia_type_t type)
{
    return type >= static_cast<gaia_type_t::value_type>(catalog_core_table_type_t::last) && type <= c_system_table_reserved_range_end;
}

inline bool is_system_object(gaia_type_t type)
{
    return type >= c_system_table_reserved_range_start && type <= c_system_table_reserved_range_end;
}

} // namespace common
} // namespace gaia
