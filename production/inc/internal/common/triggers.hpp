/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "events.hpp"
#include <stdint.h>
#include <vector>
#include <memory>
#include <atomic>
#include <functional>

#include "wait_queue.hpp"
#include "gaia_common.hpp"

using namespace gaia::common;

namespace gaia 
{
namespace db
{
namespace triggers {

struct trigger_event_t {
    event_type_t event_type; // insert, update, delete, begin, commit, rollback
    gaia_id_t gaia_type; // gaia table type, maybe 0 if event has no associated tables
    gaia_id_t record; //row id, may be 0 if if there is no assocated row id
    const u_int16_t* columns; // list of affected columns, may be null
    size_t count_columns; // count of affected columns, may be zero
};

/**
 * The type of Gaia commit trigger.
 */
typedef void (*f_commit_trigger_t) (uint64_t, std::vector<trigger_event_t>, bool);

}
}
}
