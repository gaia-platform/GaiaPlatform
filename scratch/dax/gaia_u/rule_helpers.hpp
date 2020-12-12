#include "gaia_gaia_u.h"

namespace event_planner
{
void my_func(gaia::gaia_u::Buildings_t& building);
void cancel_event(gaia::common::gaia_id_t event_id);
void move_event_room(
    gaia::gaia_u::Events_t& event,
    gaia::gaia_u::Rooms_t& old_room,
    gaia::gaia_u::Rooms_t& new_room);
bool is_room_available(
    gaia::gaia_u::Events_t& event,
    gaia::gaia_u::Rooms_t& room);
bool is_room_available(
    time_t date,
    gaia::gaia_u::Rooms_t& room);
} // namespace event_planner