/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <sstream>
#include "gaia_exception.hpp"
#include "gaia_object.hpp"

namespace gaia 
{

/**
 * \addtogroup Gaia
 * @{
 */

namespace rules
{

/**
 * \addtogroup Rules
 * @{
 * 
 * Provides facilities for mapping events to rules.
 */
 
/**
 * Every event in the system has an event type.  The type
 * is scoped to an object type. Note that values must
 * be powers of 2 and the transaction, row, and field event
 * groups must be preserved.
 */
enum class event_type_t : uint32_t {
    // Transaction events.
    transaction_begin = 1 << 0,
    transaction_commit = 1 << 1,
    transaction_rollback = 1 << 2,
    // Row events.
    row_update = 1 << 3,
    row_insert = 1 << 4,
    row_delete = 1 << 5,
};

/*@}*/
}
/*@}*/
}
