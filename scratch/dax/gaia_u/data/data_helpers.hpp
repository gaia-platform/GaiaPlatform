#pragma once

#include "gaia_gaia_u.h"

namespace event_planner
{
    extern const char* c_date_format;
    extern const char* c_time_format;

    void show_buildings();
    void delete_buildings();
    void show_building(gaia::gaia_u::Buildings_t& building);
    void show_rooms();
    void delete_rooms();
    void show_room(gaia::gaia_u::Rooms_t& room);
    void show_persons();
    void delete_persons();
    void show_person(gaia::gaia_u::Persons_t& person);
    void show_students();
    void delete_students();
    void show_parents();
    void delete_parents();
    void show_staff();
    void delete_staff();
    void show_staff_t(gaia::gaia_u::Staff_t& staff);
    void show_events();
    void delete_events();
    void show_restrictions();
    void delete_restrictions();
    void show_all();
    void delete_all();

    void update_restriction(uint8_t percent);

    int64_t convert_date(const char* date);
    std::string convert_date(int64_t datestamp);
    int64_t convert_time(const char* time);
    std::string convert_time(int64_t timestamp);
} // namespace event_planner