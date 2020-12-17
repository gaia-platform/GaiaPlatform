/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/stream/Stream.h"
#include "gaia/stream/StreamOperators.h"
#include "gaia_school.h"
#include "person_type.h"
#include "utils.h"

using namespace gaia::school;
using namespace gaia::common;
using namespace stream::op;

const static std::vector<std::string> c_student_names = {
    "Adrienne Munday",
    "Bill Clinton",
    "Brian Donovan",
    "Cameron Evans",
    "Mihir Jadhav",
    "Randall Hopkins",
};

const static std::vector<std::string> c_parent_names = {
    "Chuan Liu",
    "Dax Hawkins",
    "Gregory Fine",
    "Hal Berenson",
    "Laurentiu Cristofor",
    "Matthew Bonhage",
    "David Vaskevitch",
    "Mehtap Ozkan",
    "Nick Kline",
    "Phillip Ovanesyan",
    "Simone Rondelli",
    "Tengiz Kharatishvili",
};

const static std::vector<std::string> c_staff_names = {
    "Tobin Baker",
    "Wayne Warren",
    "Yi Wen Wong",
    "Zack Nolan",
};

persons_t create_person(std::string name, std::string surname, int64_t dob)
{
    int64_t face_signature = create_face_signature(name, surname);
    gaia_id_t person_id = persons_t::insert_row(name.c_str(), surname.c_str(), dob, face_signature, person_type::undefined);
    return persons_t::get(person_id);
}

students_t create_student(std::string name, std::string surname, int64_t dob, std::string telephone)
{
    persons_t person = create_person(name, surname, dob);
    gaia_id_t student_id = students_t::insert_row(telephone.c_str());
    person.student_person_students_list().insert(student_id);

    auto person_writer = person.writer();
    person_writer.person_type = person_type::student;
    person_writer.update_row();

    return students_t::get(student_id);
}

parents_t create_parent(std::string name, std::string surname, int64_t dob, const std::string& parent_type)
{
    persons_t person = create_person(name, surname, dob);
    gaia_id_t parent_id = parents_t::insert_row(parent_type.c_str());
    person.parent_person_parents_list().insert(parent_id);

    auto person_writer = person.writer();
    person_writer.person_type = person_type::parent;
    person_writer.update_row();

    return parents_t::get(parent_id);
}

staff_t create_staff(std::string name, std::string surname, int64_t dob, uint64_t hire_date)
{
    persons_t person = create_person(name, surname, dob);
    gaia_id_t staff_id = staff_t::insert_row(hire_date);
    person.person_staff_list().insert(staff_id);

    auto person_writer = person.writer();
    person_writer.person_type = person_type::staff;
    person_writer.update_row();

    return staff_t::get(staff_id);
}

family_t create_family(parents_t father, parents_t mother, students_t student)
{
    family_t family = family_t::get(family_t::insert_row());
    father.father_family_list().insert(family);
    mother.mother_family_list().insert(family);
    student.student_family_list().insert(family);
    return family;
}

buildings_t create_building(std::string building_name, uint64_t camera_id, bool door_closed)
{
    gaia_id_t building_id = buildings_t::insert_row(building_name.c_str(), camera_id, door_closed);
    return buildings_t::get(building_id);
}

rooms_t create_room(buildings_t building, uint64_t room_number, int64_t floor_number, int64_t capacity)
{
    std::string room_name = "room_" + std::to_string(room_number);
    gaia_id_t room_id = rooms_t::insert_row(room_number, room_name.c_str(), floor_number, capacity);
    building.building_rooms_list().insert(room_id);
    return rooms_t::get(room_id);
}

events_t create_event(std::string& name, staff_t teacher, rooms_t room)
{
    auto writer = events_writer();
    writer.name = name;
    writer.start_date = current_time_millis();
    writer.end_time = current_time_millis() + (1000 * 3600);
    writer.enrollment = 0;
    writer.event_actually_enrolled = true;
    gaia_id_t event_id = writer.insert_row();

    events_t event = events_t::get(event_id);
    teacher.teacher_events_list().insert(event);
    room.room_events_list().insert(event);
    return event;
}

void init_data()
{
    uint32_t students_count = students_t::stream()
        | count();

    if (students_count > 0)
    {
        std::cout << "DB already contain data, skipping init." << std::endl;
        return;
    }

    std::cout << "DB empty, initializing data." << std::endl;

    for (int i = 0; i < c_student_names.size(); i++)
    {
        students_t student = create_student(
            get_name(c_student_names.at(i)),
            get_surname(c_student_names.at(i)),
            1000 * i,
            "206403" + std::to_string(i * 100));

        parents_t father = create_parent(
            get_name(c_parent_names.at(i * 2)),
            get_surname(c_parent_names.at(i * 2)),
            10000 * i,
            "father");

        parents_t mother = create_parent(
            get_name(c_parent_names.at((i * 2) + 1)),
            get_surname(c_parent_names.at((i * 2) + 1)),
            10000 * i,
            "mother");

        family_t family = create_family(father, mother, student);
    }

    for (const std::string& full_name : c_staff_names)
    {
        staff_t staff = create_staff(
            get_name(full_name),
            get_surname(full_name),
            10000,
            10000);
    }

    for (int i = 1; i <= 3; i++)
    {
        buildings_t building = create_building("SEA" + std::to_string(i), i, true);

        for (int j = 1; j <= 4; j++)
        {
            create_room(building, j, j / 2, 3);
        }
    }

    std::vector<std::string> event_names{"event1", "event2", "event3"};

    for (std::string event : event_names)
    {
        auto staff = staff_t::stream()
            | random_element();

        auto room = rooms_t::stream()
            | random_element();

        create_event(event, staff, room);
    }
}
