/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cctype>
#include <chrono>

#include <algorithm>
#include <string>

void to_lower_case(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
}

int64_t create_face_signature(std::string name, std::string surname)
{
    std::string face = name + surname;
    to_lower_case(face);
    return std::hash<std::string>{}(face);
}

int64_t current_time_millis()
{
    std::chrono::milliseconds ms
        = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());
    return ms.count();
}

std::string get_name(std::string full_name)
{
    return full_name.substr(0, full_name.find_last_of(' '));
}

std::string get_surname(std::string full_name)
{
    return full_name.substr(full_name.find_last_of(' ') + 1, full_name.length());
}

bool iequals(const std::string& a, const std::string& b)
{
    return std::equal(a.begin(), a.end(), b.begin(), [](char a, char b) {
        return tolower(a) == tolower(b);
    });
}

std::string print_person(gaia::school::persons_t person)
{
    return std::string(person.first_name()) + " " + std::string(person.last_name());
}

std::string print_room(gaia::school::rooms_t room)
{
    return std::string(room.room_name()) + " floor: " + std::to_string(room.floor_number()) + " capacity: " + std::to_string(room.capacity());
}

std::string print_event(gaia::school::events_t event)
{
    std::stringstream out;
    out << "Event name: " << event.name() << std::endl
        << "Teacher: " << print_person(event.teacher_staff().person_persons()) << std::endl
        << "Room: " << event.room_rooms().building_buildings().building_name() << " " << print_room(event.room_rooms()) << std::endl
        << "Start: " << event.start_date() << std::endl
        << "End: " << event.end_time() << std::endl;
    return out.str();
}
