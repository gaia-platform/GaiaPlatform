/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <sstream>

#include "gaia_base.hpp"
#include "gaia_exception.hpp"

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
 * is scoped to an object type.
 */
enum class event_type_t : uint8_t {
    transaction_begin,
    transaction_commit,
    transaction_rollback,
    col_change,
    row_update,
    row_insert,
    row_delete,
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
        std::stringstream strs;
        strs << "Invalid mode: " << (uint8_t)mode;
        m_message = strs.str();
    }
};

/**
 * Thrown when a specified mode is not supported
 * for a log event call.  For example, if a table event type type
 * is sent in for transaction log event call, then this exception
 * is thrown.
 */
class invalid_event_type: public gaia::common::gaia_exception
{
public:
    invalid_event_type(event_type_t event_type)
    {
        std::stringstream strs;
        strs << "Invalid event type for operation: " << (uint8_t)event_type;
        m_message = strs.str();
    }
};

/**
 * Writes a table event to the event log.  If the mode is
 * event_mode_t::immediate, then the rules associated with this
 * table event are executed.  All interaction with the underlying
 * database occurs in the caller's transaction.
 * 
 * @param row current row context of this event
 * @param gaia_type type of the table the event is scoped to
 * @param event_type the type of table event that has occurred. Must be one of:
 *  (col_change, row_update, row_insert, row_delete)
 * @param mode deferred or immediate rule execution.  Only immedate rule execution
 *  is supported at this time.
 * @return true if at least one rule wwas fired due to this event; false otherwise.  
 *  Note that returning a false does not indicate an error because a rule may have 
 *  been unsubscribed from this event at runtime.
 * @throw mode_not_supported 
 * @throw invalid_event_type
 */
bool log_table_event(
    common::gaia_base* row, 
    common::gaia_type_t gaia_type,
    event_type_t event_type, 
    event_mode_t mode);

/**
 * Writes a transaction event to the event log.  If the mode is
 * event_mode_t::immediate, then the rules associated with this
 * transaction event are executed.  All interaction with the underlying
 * database occurs in the caller's transaction.
 * 
 * @param event_type the type of transcation event that has occurred.  Must be one of:
 *  (transaction_begin, transaction_commit, transactinon_rollback).
 * @param mode event_mode_t for deferred or immediate rule execution
 * @return true if at least one rule was fired due to this event; false otherwise.
 *  Note that returning a false does not indicate an error because a rule may have
 *  been unsubscribed from this event at runtime.
 * @throw mode_not_supported 
 * @throw invalid_event_type
 */
bool log_transaction_event(event_type_t event_type, event_mode_t mode);

/*@}*/
}
/*@}*/
}
