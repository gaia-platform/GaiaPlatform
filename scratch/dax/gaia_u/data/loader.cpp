/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <map>
#include <fstream>
#include <string>
#include "loader.hpp"

#include "gaia/direct_access/auto_transaction.hpp"
#include "gaia/exception.hpp"
#include "data_helpers.hpp"
#include "rule_helpers.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::gaia_u;
using namespace gaia::common;
using namespace gaia::direct_access;

// provide an option for shared memory file name
// have the client

// Simplify to just use getline and string find...
// getline does have a delimter but we want both \n and ,
// Check in sources somewhere!

// Weekend goals:

// Parse file input
// display in form with tree controls
// Python binding layer
// Create a registration form!

// Loads an input file with the following "schema"
// [#]table name, <fake id for relationships>, [table columns ...], reference to fake ids.
// if # is in the first column of the row then that row is ignored.
class csv_row_t
{
    public:
        csv_row_t(){};
        string const& operator[](size_t index) const
        {
            return _row[index];
        }

        void read_next(istream& str)
        {
            string line;
            getline(str, line);
            _row.clear();
            stringstream ss(line);
            string column;
            while (getline(ss, column, ','))
            {
                // Trim leading whitespace.  Note we dont' handle tabs currently.
                size_t pos = column.find_first_not_of(' ');
                if (pos != string::npos)
                {
                    _row.push_back(column.substr(pos));
                }
                else
                {
                    _row.push_back(column);
                }
            }
        }

        std::vector<std::string>& get_vector()
        {
            return _row;
        }

    private:
        vector<string> _row;
};

istream& operator>>(istream& str, csv_row_t& data)
{
    data.read_next(str);
    return str;
}

void gaia_u_loader_t::load_Persons(row_t& row)
{
    uint32_t id = stoul(row[1]);
    auto gaia_id = Persons_t::insert_row(
        row[2].c_str(), // FirstName
        row[3].c_str(), // LastName
        event_planner::convert_date(row[4].c_str()), //BirthDate
        nullptr //"FaceSignature"
    );

    m_persons_ids.insert(make_pair(id, gaia_id));
}

void gaia_u_loader_t::load_Buildings(row_t& row)
{
    uint32_t id = stoul(row[1]);
    bool locked = true;
    if (row[3].compare("false") == 0)
    {
        locked = false;
    }

    auto gaia_building_id = Buildings_t::insert_row(
        row[2].c_str(), // BuildingName
        locked // DoorLocked
    );

    // Add the building to the campus
    uint32_t campus_id = stoul(row[4]);
    auto gaia_campus_id = m_campus_ids[campus_id];
    if (c_invalid_gaia_id == gaia_campus_id)
    {
        throw gaia_exception("Invalid Campus id\n");
    }
    auto campus = Campus_t::get(gaia_campus_id);
    campus.Buildings_list().insert(gaia_building_id);
    m_buildings_ids.insert(make_pair(id, gaia_building_id));
}

void gaia_u_loader_t::load_Campus(row_t & row)
{
    uint32_t id = stoul(row[1]);
    auto gaia_campus_id = Campus_t::insert_row(row[2].c_str());
    m_campus_ids.insert(make_pair(id, gaia_campus_id));
}

void gaia_u_loader_t::load_Restrictions(row_t& row)
{
    uint8_t percent = stoul(row[1]);
    auto gaia_restriction_id = Restrictions_t::insert_row(percent);

    // Add the restrictions to the campus
    uint32_t campus_id = stoul(row[2]);
    auto gaia_campus_id = m_campus_ids[campus_id];
    if (c_invalid_gaia_id == gaia_campus_id)
    {
        throw gaia_exception("Invalid Campus id\n");
    }
    auto campus = Campus_t::get(gaia_campus_id);
    campus.Restrictions_list().insert(gaia_restriction_id);
}

void gaia_u_loader_t::load_Rooms(row_t& row)
{
    uint32_t id = stoul(row[1]);
    uint16_t number = static_cast<uint16_t>(stoul(row[2]));
    uint8_t floor = static_cast<uint8_t>(stoul(row[4]));
    uint16_t capacity = static_cast<uint16_t>(stoul(row[5]));
    uint16_t restricted = static_cast<uint16_t>(stoul(row[6]));

    auto gaia_room_id = Rooms_t::insert_row(
        number, // RoomNumber
        row[3].c_str(), //RoomName
        floor, //FloorNumber
        capacity, //Capacity
        restricted //RestrictedCapacity
    );

    // Insert the room into the referenced building
    uint32_t building_id = stoul(row[7]);
    auto gaia_building_id = m_buildings_ids[building_id];
    if (c_invalid_gaia_id == gaia_building_id)
    {
        throw gaia_exception("Invalid Building id\n");
    }
    auto building = Buildings_t::get(gaia_building_id);
    building.Rooms_list().insert(gaia_room_id);
    m_rooms_ids.insert(make_pair(id, gaia_room_id));
}

void gaia_u_loader_t::load_Students(row_t& row)
{
    uint32_t id = stoul(row[1]);
    uint16_t number = static_cast<uint16_t>(stoul(row[2]));
    auto gaia_student_id = Students_t::insert_row(number);

    // Insert the student into the referenced Person
    uint32_t person_id = stoul(row[3]);
    auto gaia_person_id = m_persons_ids[person_id];
    if (c_invalid_gaia_id == gaia_person_id)
    {
        throw gaia_exception("Invalid Person id\n");
    }
    auto person = Persons_t::get(gaia_person_id);
    person.Students_list().insert(gaia_student_id);
    m_students_ids.insert(make_pair(id, gaia_student_id));
}

void gaia_u_loader_t::load_Staff(row_t& row)
{
    uint32_t id = stoul(row[1]);
    auto gaia_staff_id = Staff_t::insert_row(
        event_planner::convert_date(row[2].c_str())); //BirthDate

    // Insert the staff into the referenced Person
    uint32_t person_id = stoul(row[3]);
    auto gaia_person_id = m_persons_ids[person_id];
    if (c_invalid_gaia_id == gaia_person_id)
    {
        throw gaia_exception("Invalid Person id\n");
    }
    auto person = Persons_t::get(gaia_person_id);
    person.Staff_list().insert(gaia_staff_id);
    m_staff_ids.insert(make_pair(id, gaia_staff_id));
}

void gaia_u_loader_t::load_Registrations(row_t& row)
{
    //uint32_t id = stoul(row[1]);
    auto gaia_registration_id = Registrations_t::insert_row(
        event_planner::convert_date(row[1].c_str()), // RegistrationDate
        event_planner::convert_time(row[2].c_str()),
        false); // NotifyDrop

    // Add Event
    uint32_t event_id = stoul(row[3]);
    auto gaia_event_id = m_events_ids[event_id];
    if (c_invalid_gaia_id == gaia_event_id)
    {
        throw gaia_exception("Invalid event id\n");
    }
    auto event = Events_t::get(gaia_event_id);
    event.Registrations_list().insert(gaia_registration_id);

    // Add Student
    uint32_t student_id = stoul(row[4]);
    auto gaia_student_id = m_students_ids[student_id];
    if (c_invalid_gaia_id == gaia_student_id)
    {
        throw gaia_exception("Invalid student id\n");
    }
    auto student = Students_t::get(gaia_student_id);
    student.Registrations_list().insert(gaia_registration_id);
}

void gaia_u_loader_t::load_Events(row_t& row)
{
    uint32_t id = stoul(row[1]);
    auto gaia_event_id = Events_t::insert_row(
        row[2].c_str(), // Name
        event_planner::convert_date(row[3].c_str()), //Date
        event_planner::convert_time(row[4].c_str()), //StartTime
        event_planner::convert_time(row[5].c_str()), //EndTime
        0, // Enrolled
        false, // ChangeLocation
        false, // ChangeDate 
        false, // DropEnrollments
        event_planner::notify_reason_t::none // NotifyDate
    );

    // Insert the event for the staff
    uint32_t staff_id = stoul(row[6]);
    auto gaia_staff_id = m_staff_ids[staff_id];
    auto staff = Staff_t::get(gaia_staff_id);
    staff.Events_list().insert(gaia_event_id);

    // Insert the event for the room
    uint32_t room_id = stoul(row[7]);
    auto gaia_room_id = m_rooms_ids[room_id];
    auto room = Rooms_t::get(gaia_room_id);
    room.Events_list().insert(gaia_event_id);
    m_events_ids.insert(make_pair(id, gaia_event_id));
}

void gaia_u_loader_t::load_Parents(row_t& row)
{
    uint32_t id = stoul(row[1]);
    auto gaia_parent_id = Parents_t::insert_row(row[2].c_str());

    // Insert the Parents into the referenced Person
    uint32_t person_id = stoul(row[3]);
    auto gaia_person_id = m_persons_ids[person_id];
    if (c_invalid_gaia_id == gaia_person_id)
    {
        throw gaia_exception("Invalid Person id\n");
    }
    auto person = Persons_t::get(gaia_person_id);
    person.Parents_list().insert(gaia_parent_id);
    m_parents_ids.insert(make_pair(id, gaia_parent_id));
}

bool gaia_u_loader_t::load(const char* data_file)
{
    ifstream f(data_file);
    if (f.fail())
    {
        printf("Could not open '%s'\n", data_file);
        return false;
    }

    uint32_t count_persons = 0;
    uint32_t count_events = 0;
    uint32_t count_buildings = 0;
    uint32_t count_rooms = 0;
    uint32_t count_students = 0;
    uint32_t count_parents = 0;
    uint32_t count_staff = 0;
    uint32_t count_restrictions = 0;
    uint32_t count_registrations = 0;
    uint32_t count_campus = 0;

    csv_row_t row;
    {
        auto_transaction_t tx(auto_transaction_t::no_auto_begin);

        while (f >> row)
        {
            string first = row[0];

            // Ignore rows with a comment.
            if (first.front() == '#')
            {
                continue;
            }

            auto data = row.get_vector();

            if (row[0].compare("Persons") == 0)
            {
                load_Persons(data);
                count_persons++;
            }
            else
            if (row[0].compare("Buildings") == 0)
            {
                load_Buildings(data);
                count_buildings++;
            }
            else
            if (row[0].compare("Rooms") == 0)
            {
                load_Rooms(data);
                count_rooms++;
            }
            else
            if (row[0].compare("Students") == 0)
            {
                load_Students(data);
                count_students++;
            }
            else
            if (row[0].compare("Parents") == 0)
            {
                load_Parents(data);
                count_parents++;
            }
            else
            if (row[0].compare("Staff") == 0)
            {
                load_Staff(data);
                count_staff++;
            }
            else
            if (row[0].compare("Events") == 0)
            {
                load_Events(data);
                count_events++;
            }
            else
            if (row[0].compare("Restrictions") == 0)
            {
                load_Restrictions(data);
                count_restrictions++;
            }
            else
            if (row[0].compare("Registrations") == 0)
            {
                load_Registrations(data);
                count_registrations++;
            }            
            else
            if (row[0].compare("Campus") == 0)
            {
                load_Campus(data);
                count_campus++;
            }
        }
        tx.commit();
    }
    printf("Added: \n%u Campus\n%u Persons\n%u Events\n%u Buildings\n%u Rooms\n%u Students\n"
        "%u Parents\n%u Staff\n%u Restrictions\n%u Registrations\n", 
        count_campus, count_persons, count_events,
        count_buildings, count_rooms, count_students,
        count_parents, count_staff, count_restrictions, count_registrations);

    return true;
}
