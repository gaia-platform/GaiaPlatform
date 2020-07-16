/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "events.hpp"
#include <stdint.h>
#include <vector>
#include <memory>

namespace gaia 
{
namespace db
{
namespace triggers {

struct trigger_event_t {
    event_type_t event_type; // insert, update, delete, begin, commit, rollback
    uint64_t gaia_type; // gaia table type, maybe 0 if event has no associated table
    uint64_t record; //row id, may be 0 if if there is no assocated row id
    const uint16_t* columns; // list of affected columns, may be null
    size_t count_columns; // count of affected columsn, may be zero
};

class event_trigger {
public:
event_trigger() {}
/**
* Internal trigger function that is called by the high level storage engine when
* a commit occurs.  Trigger events can be queued by setting immediate == false.
* 
* @param tx_id Transaction id that has just been committed.  Currently unused.
* @param events All the events that were part of this transaction.  May be null if this commit had no events.
* @param count_events May be 0 if this commit had no events.
* @param immediate True if events should be fired immediately. Default is false.
*/ 
static void commit_trigger_(int64_t tx_id, std::shared_ptr<std::vector<std::unique_ptr<triggers::trigger_event_t>>> events, size_t count_events, bool immediate = false);

};

}
}
}
