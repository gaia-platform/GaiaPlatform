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

#include "gaia_common.hpp"
#include "gaia_db_internal.hpp"
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
    gaia_type_t gaia_type; // gaia table type, maybe 0 if event has no associated tables
    gaia_id_t record; //row id, may be 0 if if there is no assocated row id
    field_position_list_t columns; // list of affected columns

    trigger_event_t(event_type_t event_type, gaia_type_t gaia_type, gaia_id_t record, field_position_list_t columns):
        event_type(event_type), gaia_type(gaia_type), record(record), columns(columns) {}
};

typedef std::vector<trigger_event_t> trigger_event_list_t;

/**
 * The type of Gaia commit trigger.
 */
typedef void (*commit_trigger_fn)(gaia_xid_t xid, const trigger_event_list_t&);

/**
 * Use this constant to specify that no fields are needed when generating trigger_event_t.
 */
const field_position_list_t empty_position_list;

} // namespace triggers

/**
 * Set by the rules engine.
 */
extern triggers::commit_trigger_fn s_tx_commit_trigger;

} // namespace db
} // namespace gaia
