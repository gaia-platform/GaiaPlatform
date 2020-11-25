/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia/rules/rules.hpp"

using namespace gaia::common;

namespace gaia
{
namespace rules
{

// Note: this function is not used anywhere outside this module hence the linker will not export
// it. The linker does not consider single functions but entire object files for export
// (unless specified otherwise). The SDK uses the "-Wl,--whole-archive" directive to
// force the export of this function
last_operation_t rule_context_t::last_operation(gaia_type_t other_gaia_type) const
{
    last_operation_t operation = last_operation_t::none;

    if (other_gaia_type != gaia_type)
    {
        return operation;
    }

    switch (event_type)
    {
    case db::triggers::event_type_t::row_delete:
        operation = last_operation_t::row_delete;
        break;
    case db::triggers::event_type_t::row_insert:
        operation = last_operation_t::row_insert;
        break;
    case db::triggers::event_type_t::row_update:
        operation = last_operation_t::row_update;
        break;
    default:
        // Ignore other event types.
        break;
    }

    return operation;
}

} // namespace rules
} // namespace gaia
