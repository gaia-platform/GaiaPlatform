/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <stdint.h>
#include <vector>
#include <memory>
#include <atomic>
#include <functional>

#include "events.hpp"
#include "types.hpp"

using namespace gaia::common;
using namespace gaia::db::types;

namespace gaia 
{
namespace db
{
namespace triggers {

struct trigger_event_t {
    event_type_t event_type; // insert, update, delete, begin, commit, rollback
    gaia_id_t gaia_type; // gaia table type, maybe 0 if event has no associated tables
    gaia_id_t record; //row id, may be 0 if if there is no assocated row id
    const field_position_t* columns; // list of affected columns, may be null
    size_t count_columns; // count of affected columns, may be zero
};

typedef std::vector<trigger_event_t> trigger_event_list_t;

/**
 * The type of Gaia commit trigger.
 */
typedef void (*commit_trigger_fn) (uint64_t, trigger_event_list_t);

}
}
}
