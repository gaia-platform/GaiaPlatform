/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <stdint.h>
#include <utility>
#include <vector>
#include <memory>
#include <atomic>
#include <functional>

#include "gaia_common.hpp"
#include "events.hpp"

using namespace gaia::common;
using namespace gaia::db;

namespace gaia
{
namespace db
{
namespace triggers {

struct trigger_event_t {
    event_type_t event_type; // insert, update, delete, begin, commit, rollback
    gaia_id_t container; // gaia table type, maybe 0 if event has no associated tables
    gaia_id_t record; //row id, may be 0 if there is no associated row id
    field_position_list_t columns; // list of affected columns

    trigger_event_t(event_type_t event_type, gaia_id_t container, gaia_id_t record, field_position_list_t columns):
        event_type(event_type), container(container), record(record), columns(std::move(columns)) {}
};

typedef std::vector<trigger_event_t> trigger_event_list_t;

/**
 * The type of Gaia commit trigger.
 */
typedef void (*commit_trigger_fn) (uint64_t, const trigger_event_list_t&);

/**
 * Use this constant to specify that no fields are needed when generating trigger_event_t.
 */ 
const field_position_list_t empty_position_list;

}
}
}
