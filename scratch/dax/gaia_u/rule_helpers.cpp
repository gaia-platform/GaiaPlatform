#include <mutex>
#include <thread>
#include <iostream>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <chrono>

#include "gaia_gaia_u.h"
#include "gaia/logger.hpp"
#include "rule_helpers.hpp"

using namespace std::chrono;

//void my_func(gaia::gaia_u::Buildings_t& building);
void event_planner::cancel_event(gaia::common::gaia_id_t event_id)
{
    gaia_log::app().info("Deleted event:  {}\n", event_id);
}

void event_planner::move_event_room(
    gaia::gaia_u::Events_t& event,
    gaia::gaia_u::Rooms_t& old_room,
    gaia::gaia_u::Rooms_t& new_room)
{
    const char * old_room_name = old_room ? old_room.RoomName() : "<none>";
    gaia_log::app().info("Moving event '{}' from room '{}' to room '{}'.",
        event.Name(), old_room_name, new_room.RoomName());

    if (old_room)
    {
        if (!(old_room.Buildings() == new_room.Buildings()))
        {
            gaia_log::app().info("Note: The building has changed from '{}' to '{}'.",
                old_room.Buildings().BuildingName(), new_room.Buildings().BuildingName());
        }

        gaia_log::app().info("Erase '{}' from '{}'", event.Name(), old_room.RoomName());
        old_room.Events_list().remove(event);
    }
    gaia_log::app().info("Insert '{}' into '{}'", event.Name(), new_room.RoomName());
    new_room.Events_list().insert(event);
}


// Determine whether this room is free for the 
// time this event is scheduled for.
bool event_planner::is_room_available(
    gaia::gaia_u::Events_t& event,
    gaia::gaia_u::Rooms_t& room)
{
    int64_t event_date = event.Date();

    // Walk through all the events this room is booked
    for (auto other_event : room.Events_list())
    {
        if (event_date == other_event.Date()) 
        {
            int64_t start_time = event.StartTime();
            int64_t end_time = event.EndTime();

            // Same day.  Is there a conflict?
            if ((other_event.EndTime() > start_time) &&
                (other_event.StartTime() < end_time))
            {
                gaia_log::app().info("Room Conflict:  Event '{}' conflicts with event '{}' for room '{}'.",
                    event.Name(), other_event.Name(), room.RoomName());
                return false;
            }
        }
    }

    return true;
}

bool event_planner::is_room_available(
    time_t date,
    gaia::gaia_u::Rooms_t& room)
{
    // Walk through all the events this room is booked
    for (auto other_event : room.Events_list())
    {
        if (date == other_event.Date()) 
        {
            return false;
        }
    }

    return true;
}
