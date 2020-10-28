/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

#include "db_types.hpp"
#include "events.hpp"
#include "gaia_common.hpp"

using namespace gaia::common;
using namespace gaia::db;

namespace gaia
{
namespace db
{
namespace triggers
{

struct trigger_event_t
{
    event_type_t event_type;

    // Can 0 if event has no associated tables.
    gaia_type_t gaia_type;

    // Can be 0 if if there is no associated row id.
    gaia_id_t record;

    field_position_list_t columns;

    trigger_event_t(event_type_t event_type, gaia_type_t gaia_type, gaia_id_t record, field_position_list_t columns)
        : event_type(event_type), gaia_type(gaia_type), record(record), columns(columns)
    {
    }
};

typedef std::vector<trigger_event_t> trigger_event_list_t;

/**
 * The type of Gaia commit trigger.
 */
typedef void (*commit_trigger_fn)(gaia::db::gaia_txn_id_t, const trigger_event_list_t&);

/**
 * Use this constant to specify that no fields are needed when generating trigger_event_t.
 */
const field_position_list_t empty_position_list = {};

} // namespace triggers
} // namespace db
} // namespace gaia
