/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <sstream>
#include "events.hpp"

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
 * Provides mock facility for firing the after commit
 * trigger for testing.
 */

struct trigger_event_t {
    event_type_t event_type; // insert, update, delete, begin, commit, rollback
    gaia_type_t gaia_type; // gaia table type, maybe 0 if event has no associated table
    gaia_id_t  record; //row id, may be 0 if if there is no assocated row id
    const uint16_t* columns; // list of affected columns, may be null
    uint16_t count_columns; // count of affected columsn, may be zero
};

/**
* Internal trigger function that is called by the high level storage engine when
* a commit occurs.
* 
* @param tx_id Transaction id that has just been committed.  Currently unused.
* @param events All the events that were part of this transaction.  May be null if this commit had no events.
* @param count_events May be 0 if this commit had no events.
* @param immediate True if events should be fired immediately. Default is false.
*/ 
void commit_trigger(uint32_t tx_id, trigger_event_t* events, size_t count_events, bool immediate = false);

 
/*@}*/
}
/*@}*/
}
