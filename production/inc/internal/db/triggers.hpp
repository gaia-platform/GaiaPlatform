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
using namespace gaia::db;

namespace gaia
{
namespace db
{
namespace triggers {

typedef std::vector<field_position_t> field_pos_list_t;

typedef std::shared_ptr<field_pos_list_t> field_list_ptr_t;

struct trigger_event_t {
    event_type_t event_type; // insert, update, delete, begin, commit, rollback
    gaia_id_t gaia_type; // gaia table type, maybe 0 if event has no associated tables
    gaia_id_t record; //row id, may be 0 if if there is no assocated row id
    field_position_list_t columns; // list of affected columns

    trigger_event_t(event_type_t event_type, gaia_id_t gaia_type, gaia_id_t record, field_position_list_t columns):
        event_type(event_type), gaia_type(gaia_type), record(record), columns(columns) {}
};

typedef std::vector<trigger_event_t> trigger_event_list_t;

/**
 * The type of Gaia commit trigger.
 */
typedef void (*commit_trigger_fn) (uint64_t, trigger_event_list_t);

/**
 * Use this constant to specify that no fields are needed when generating trigger_event_t.
 */ 
const field_position_list_t empty_position_list;

}
}
}
