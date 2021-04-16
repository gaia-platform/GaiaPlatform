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

#include "gaia/common.hpp"
#include "gaia/events.hpp"

#include "gaia_internal/db/db_types.hpp"

namespace gaia
{
namespace db
{
namespace triggers
{

struct trigger_event_t
{
    event_type_t event_type;

    // Can be 0 if event has no associated tables.
    common::gaia_type_t gaia_type;

    // Can be 0 if if there is no associated row id.
    common::gaia_id_t record;

    common::field_position_list_t columns;

    // The id f the transaction that generated this event.
    gaia_txn_id_t txn_id;

    trigger_event_t(event_type_t event_type, common::gaia_type_t gaia_type, common::gaia_id_t record, common::field_position_list_t columns, gaia_txn_id_t txn_id)
        : event_type(event_type), gaia_type(gaia_type), record(record), columns(columns), txn_id(txn_id)
    {
    }
};

typedef std::vector<trigger_event_t> trigger_event_list_t;

/**
 * The type of Gaia commit trigger.
 */
typedef void (*commit_trigger_fn)(const trigger_event_list_t&);

/**
 * Use this constant to specify that no fields are needed when generating trigger_event_t.
 */
const common::field_position_list_t empty_position_list = {};

} // namespace triggers
} // namespace db
} // namespace gaia
