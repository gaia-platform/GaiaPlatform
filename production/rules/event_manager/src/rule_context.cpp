/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "rules.hpp"

using namespace gaia::rules;
using namespace gaia::common;

last_operation_t rule_context_t::last_operation(gaia_type_t other_gaia_type)
{
    last_operation_t operation = last_operation_t::none;

    if (other_gaia_type != gaia_type)
    {
        return operation;
    }

    switch(event_type)
    {
        case event_type_t::row_delete:
            operation = last_operation_t::row_delete;
            break;
        case event_type_t::row_insert:
            operation = last_operation_t::row_insert;
            break;
        case event_type_t::row_update:
            operation = last_operation_t::row_update;
            break;
        default:
            // Ignore other event types.
            break;
    }

    return operation;
}
