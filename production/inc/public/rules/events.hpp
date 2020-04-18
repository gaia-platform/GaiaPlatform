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
 * Provides facilities for logging events to the system.
 */
 
 /**
  * Immediate and deferred specify when the rules
  * associated with the event should be executed.
  */
enum class event_mode_t : uint8_t {
    immediate, /**<execute the rule when the event is logged */
    deferred, /**<execute the event at later time */
};

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
    // Field events.
    field_read = 1 << 6,
    field_write = 1 << 7,
    // Labels.
    first_row_event = row_update,
    first_field_event = field_read,
};

/**
 * Thrown when a specified mode is not supported
 * for a log event call.
 */
class mode_not_supported: public gaia::common::gaia_exception
{
public:
    mode_not_supported(event_mode_t mode)
    {
        std::stringstream message;
        message << "Invalid mode: " << (uint8_t)mode;
        m_message = message.str();
    }
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

/**
 * Writes a database event to the event log.  If the mode is
 * event_mode_t::immediate, then the rules associated with this
 * database event are executed.  All interaction with the underlying
 * database occurs in the caller's transaction.
 * 
 * @param row Current row context of this event.  May be null.
  * @param event_type The type of table event that has occurred.
 * @param mode deferred or immediate rule execution.  Only immedate rule execution
 *  is supported at this time.
 * @return true if at least one rule wwas fired due to this event; false otherwise.  
 *  Note that returning a false does not indicate an error because a rule may have 
 *  been unsubscribed from this event at runtime.
 * @throw mode_not_supported 
 * @throw initialization_error
 */
bool log_database_event(
    common::gaia_base_t* row, 
    event_type_t event_type, 
    event_mode_t mode);

/**
 * Writes a field event to the event log.  If the mode is
 * event_mode_t::immediate, then the rules associated with this
 * field event are executed.  All interaction with the underlying
 * database occurs in the caller's transaction.
 * 
 * @param row Current row context of this event.
 * @param field The field that is the source of this event
 * @param event_type The type of table event that has occurred.
 * @param mode deferred or immediate rule execution.  Only immedate rule execution
 *  is supported at this time.
 * @return true if at least one rule wwas fired due to this event; false otherwise.  
 *  Note that returning a false does not indicate an error because a rule may have 
 *  been unsubscribed from this event at runtime.
 * @throw mode_not_supported 
 * @throw invalid_event_type
 * @throw initialization_error
 */
bool log_field_event(
    common::gaia_base_t* row,
    const char* field,
    event_type_t event_type, 
    event_mode_t mode);

/*@}*/
}
/*@}*/
}
