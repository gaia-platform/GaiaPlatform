/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "gaia_base.hpp"

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
enum class event_mode_t {
    immediate, /**<execute the rule when the event is logged */
    deferred, /**<execute the event at later time */
};

/**
 * Every event in the system has an event type.  The type
 * is scoped to an object type.
 */
enum class event_type_t {
    transaction_begin,
    transaction_commit,
    transaction_rollback,
    col_change,
    row_update,
    row_insert,
    row_delete,
};

/**
 * Writes a table event to the event log.  If the mode is
 * event_mode_t::immediate, then the rules associated with this
 * table event are executed.  All interaction with the underlying
 * database occurs in the callers transaction.
 * 
 * @param row current row context of this event
 * @param gaia_type type of the table the event is scoped to
 * @param event_type the type of table event that has occurred
 * @param mode deferred or immediate rule execution
 * @throw mode_not_supported
 * @return true if an event was logged; false otherwise.  An event may
 *  not have been logged if the gaia_type was not found, for example
 *      type of the event is not one of event_type::[col_change, row_update,
 *      row_insert, row_delete]
 */
gaia::common::error_code_t log_table_event(
    common::gaia_base* row, 
    common::gaia_type_t gaia_type,
    event_type_t event_type, 
    event_mode_t mode);

/**
 * Writes a transaction event to the event log.  If the mode is
 * event_mode_t::immediate, then the rules associated with this
 * transaction event are executed.  All interaction with the underlying
 * database occurs in the callers transaction.
 * 
 * @param type the type of transcation event that has occurred
 * @param mode event_mode_t for deferred or immediate rule execution
 * @return true if succesful, failure if the type is not one of
 *      event_type::[transaction_begin, transaction_commit,
 *      transaction_rollback]
 */
gaia::common::error_code_t log_transaction_event(event_type_t type, event_mode_t mode);

/*@}*/
}
/*@}*/
}
