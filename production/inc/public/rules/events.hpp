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

/**
 * Thrown when the correct context information is not supplied for the
 * log event call.  Database events must have no context, row events
 * must have a row context, and field events must have both a row context
 * and a field name.
 */
class invalid_context: public gaia::common::gaia_exception
{
public:
    invalid_context()
    {
        m_message = "Transaction events must not have an associated row context.";
    }
    invalid_context(const gaia_base_t*)
    {
        m_message = "Row events must have an associated row context.";
    }
    invalid_context(const gaia_base_t*, const char*)
    {
        m_message = "Field events must have both a row context and field name.";
    }
};

/**
 * Thrown when a specified event_type does not match the
 * log event call.  For example, if a field event type
 * is passed in for database log event call, then this exception
 * is thrown.
 */
class invalid_event_type: public gaia::common::gaia_exception
{
public:
    invalid_event_type(event_type_t event_type)
    {
        std::stringstream message;
        message << "Invalid event type for operation: " << (uint32_t)event_type;
        m_message = message.str();
    }
};
/*@}*/
}
/*@}*/
}
