/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia/events.hpp"

#include "gaia_internal/common/retail_assert.hpp"

namespace gaia
{
namespace db
{
namespace triggers
{

static constexpr char c_event_type_not_set[] = "not_set";
static constexpr char c_event_type_row_update[] = "row_update";
static constexpr char c_event_type_row_insert[] = "row_insert";
//TODO[GAIAPLAT-445] We don't expose deleted row events
//static constexpr char c_event_type_row_delete[] = "row_delete";

const char* event_type_name(event_type_t event_type)
{
    switch (event_type)
    {
    case event_type_t::row_update:
        return c_event_type_row_update;
    //TODO[GAIAPLAT-445] We don't expose deleted row events
    //case event_type_t::row_delete:
    //    return c_event_type_row_delete;
    case event_type_t::row_insert:
        return c_event_type_row_insert;
    case event_type_t::not_set:
        return c_event_type_not_set;
    default:
        ASSERT_UNREACHABLE("Unknown event type!");
    }
}

} // namespace triggers
} // namespace db
} // namespace gaia
