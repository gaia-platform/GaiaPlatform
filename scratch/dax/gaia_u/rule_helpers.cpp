#include "gaia_gaia_u.h"
#include "rule_helpers.hpp"

//void my_func(gaia::gaia_u::Buildings_t& building);
void event_planner::cancel_event(gaia::common::gaia_id_t event_id)
{
    printf("deleted event:  %lu\n", event_id);
}

void event_planner::move_event_room(
    gaia::gaia_u::Events_t& event,
    gaia::gaia_u::Rooms_t& old_room,
    gaia::gaia_u::Rooms_t& new_room)
{

    const char * old_room_name = old_room ? old_room.RoomName() : "<none>";
    printf("Moving event '%s' from room '%s' to room '%s'\n",
        event.Name(), old_room_name, new_room.RoomName());

    if (old_room)
    {
        if (!(old_room.Buildings() == new_room.Buildings()))
        {
            printf("Note!  The building has changed from '%s' to '%s'.\n",
                old_room.Buildings().BuildingName(), new_room.Buildings().BuildingName());
        }

        old_room.Room_Events_list().erase(event);
    }
    new_room.Room_Events_list().insert(event);
}


// Determine whether this room is free for the 
// time this event is scheduled for.
bool event_planner::is_room_available(
    gaia::gaia_u::Events_t& event,
    gaia::gaia_u::Rooms_t& room)
{
    int64_t event_date = event.Date();

    // Walk through all the events this room is booked
    for (auto other_event : room.Room_Events_list())
    {
        if (event_date == other_event.Date()) 
        {
            int64_t start_time = event.StartTime();
            int64_t end_time = event.EndTime();

            // Same day.  Is there a conflict?
            if ((other_event.EndTime() > start_time) &&
                (other_event.StartTime() < end_time))
            {
                printf("Found conflict!  Event '%s' conflicts with event '%s'\n",
                    event.Name(), other_event.Name());
                    
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
    for (auto other_event : room.Room_Events_list())
    {
        if (date == other_event.Date()) 
        {
            return false;
        }
    }

    return true;
}