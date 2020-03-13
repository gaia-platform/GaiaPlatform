/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "gaia_obj.hpp"

namespace gaia 
{

/**
 * \addtogroup Gaia
 * @{
 */

namespace events
{

/**
 * \addtogroup Events
 * @{
 * 
 * Provides facilities for logging events to the system.
 */
 
 /**
  * Immediate and deferred specify when the rules
  * associated with the event should be executed.
  */
enum class event_mode {
    immediate, /**<execute the rule when the event is logged */
    deferred, /**<execute the event at later time */
};

/**
 * Every event in the system has an event type.  The type
 * is scoped to an object type.
 */
enum class event_type {
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
 * event_mode::immediate, then the rules associated with this
 * table event are executed.  All interaction with the underlying
 * database occurs in the callers transaction.
 * 
 * @param row current row context of this event
 * @param type the type of table event that has occurred
 * @param mode deferred or immediate rule execution
 * @return true if succesful, failure if a row is not provided or the 
 *      type of the event is not one of event_type::[col_change, row_update,
 *      row_insert, row_delete]
 */
bool log_table_event(common::gaia_base_t* row, event_type type, event_mode mode);

/**
 * Writes a transaction event to the event log.  If the mode is
 * event_mode::immediate, then the rules associated with this
 * transaction event are executed.  All interaction with the underlying
 * database occurs in the callers transaction.
 * 
 * @param type the type of transcation event that has occurred
 * @param mode event_mode for deferred or immediate rule execution
 * @return true if succesful, failure if the type is not one of
 *      event_type::[transaction_begin, transaction_commit,
 *      transaction_rollback]
 */
bool log_transaction_event(event_type type, event_mode mode);

/*@}*/
}
/*@}*/
}
