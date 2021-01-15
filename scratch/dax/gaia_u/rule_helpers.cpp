#include <mutex>
#include <thread>
#include <iostream>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <chrono>

#include "gaia_gaia_u.h"
#include "rule_helpers.hpp"

using namespace std::chrono;

namespace event_planner 
{
    bool is_verbose = false;
}

void event_planner::log(const char* text, bool is_verbose)
{
    // If this log entry is only verbose mode and we are not in verbose
    // mode then don't output anything
    if (is_verbose && !event_planner::is_verbose)
    {
        return;
    }

    if (event_planner::is_verbose)
    {
        pid_t tid;
        tid = syscall(SYS_gettid);
        milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
        printf("[%lu][%d] %s\n", ms.count(), tid, text);
    }
    else
    {
        printf("%s\n", text);
    }
}

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
    char buffer[128];
    // Are insertions and deletions not transaction safe?
    //std::unique_lock lock(safe_move);

    //std::cout << "thread: " << std::this_thread::get_id() << std::endl;

    const char * old_room_name = old_room ? old_room.RoomName() : "<none>";
    sprintf(buffer, "Moving event '%s' from room '%s' to room '%s'.",
        event.Name(), old_room_name, new_room.RoomName());
    event_planner::log(buffer);

    if (old_room)
    {
        if (!(old_room.Buildings() == new_room.Buildings()))
        {
            sprintf(buffer, "Note: The building has changed from '%s' to '%s'.",
                old_room.Buildings().BuildingName(), new_room.Buildings().BuildingName());
            event_planner::log(buffer);                
        }

        sprintf(buffer, "Erase '%s' from '%s'", event.Name(), old_room.RoomName());
        event_planner::log(buffer, true);
        old_room.Events_list().erase(event);
    }
    sprintf(buffer, "Insert '%s' into '%s'", event.Name(), new_room.RoomName());
    event_planner::log(buffer, true);
    new_room.Events_list().insert(event);
}


// Determine whether this room is free for the 
// time this event is scheduled for.
bool event_planner::is_room_available(
    gaia::gaia_u::Events_t& event,
    gaia::gaia_u::Rooms_t& room)
{
    char buffer[128];
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
                sprintf(buffer, "Room Conflict:  Event '%s' conflicts with event '%s' for room '%s'.",
                    event.Name(), other_event.Name(), room.RoomName());
                event_planner::log(buffer);
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
