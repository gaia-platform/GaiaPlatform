
#include "gaia_gaia_u.h"
#include "loader.hpp"
#include "data_helpers.hpp"
#include "gaia/system.hpp"
#include "gaia/rules/rules.hpp"
#include "gaia/direct_access/auto_transaction.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::direct_access;
using namespace gaia::gaia_u;
using namespace gaia::rules;

using namespace event_planner;

const char* event_planner::c_date_format = "%m/%d/%Y";
const char* event_planner::c_time_format = "%I:%M %p";

int64_t event_planner::convert_date(const char* date)
{
    tm the_date;
    memset(&the_date, 0, sizeof(tm));
    strptime(date, event_planner::c_date_format, &the_date);
    return mktime(&the_date);
}

string event_planner::convert_date(int64_t datestamp)
{
    char buffer[12];
    tm* t_ptr = localtime(&datestamp);
    strftime(buffer, sizeof(buffer), event_planner::c_date_format, t_ptr);
    return string(buffer);
}

int64_t event_planner::convert_time(const char* time)
{
    tm the_time;
    memset(&the_time, 0, sizeof(tm));
    strptime(time, event_planner::c_time_format, &the_time);
    return mktime(&the_time);
}

int64_t event_planner::get_timestamp(int64_t datestamp, int64_t timestamp)
{
    tm ts; // timestamp
    memset(&ts, 0, sizeof(tm));

    tm* tm_ptr = localtime(&datestamp);
    ts.tm_mday = tm_ptr->tm_mday;
    ts.tm_mon = tm_ptr->tm_mon;
    ts.tm_year = tm_ptr->tm_year;
    tm_ptr = localtime(&timestamp);
    ts.tm_sec = tm_ptr->tm_sec;
    ts.tm_min = tm_ptr->tm_min;
    ts.tm_hour = tm_ptr->tm_hour;
    return mktime(&ts);
}

string event_planner::convert_time(int64_t timestamp)
{
    char buffer[12];
    tm* t_ptr = localtime(&timestamp);
    strftime(buffer, sizeof(buffer), event_planner::c_time_format, t_ptr);
    return string(buffer);
}

void event_planner::show_campus_t(Campus_t& campus)
{
    printf("%lu: %s", campus.gaia_id(), campus.Name());
}

void event_planner::show_campus()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    printf("Campus Table\n");
    printf("--------\n");
    for (auto c : Campus_t::list())
    {
        show_campus_t(c);
        printf("\n");
    }
    printf("--------\n");
}

void event_planner::delete_campus()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    for (auto campus = Campus_t::get_first() ; campus ; campus = Campus_t::get_first())
    {
        campus.delete_row();
    }
    tx.commit();
}

void event_planner::show_building(Buildings_t& building)
{
    auto c= building.Campus();
    printf("%lu: %s [Campus]{", building.gaia_id(), building.BuildingName());
    show_campus_t(c);
    printf("}");
}

void event_planner::show_buildings()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    printf("Buildings Table\n");
    printf("--------\n");
    for (auto b : Buildings_t::list())
    {
        show_building(b);
        printf("\n");
    }
    printf("--------\n");
}

void event_planner::delete_buildings()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    for (auto building = Buildings_t::get_first() ; building ; building = Buildings_t::get_first())
    {
        //Disconnect from campus
        auto campus = building.Campus();
        campus.Buildings_list().erase(building);
        building.delete_row();
    }
    tx.commit();
}

void event_planner::show_room(Rooms_t& room)
{
    if (!room)
    {
        printf("<none>");
        return;
    }
    auto b = room.Buildings();
    printf("%lu: %u, %s, %u, %u %u [Building]{",
        room.gaia_id(), room.RoomNumber(), room.RoomName(), room.FloorNumber(), room.Capacity(), room.RestrictedCapacity());
    show_building(b);
    printf("}");
}

void event_planner::delete_rooms()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    for (auto room = Rooms_t::get_first() ; room ; room = Rooms_t::get_first())
    {
        // Disconnnect from any buildings
        room.Buildings().Rooms_list().erase(room);
        // now delete
        room.delete_row();
    }
    tx.commit();
}

void event_planner::show_rooms()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    printf("Rooms Table\n");
    printf("--------\n");
    for (auto r : Rooms_t::list())
    {
        show_room(r);
        printf("\n");
    }
    printf("--------\n");
}

void event_planner::show_person(Persons_t& person)
{
    printf("%lu: %s %s, %s",
        person.gaia_id(),
        person.FirstName(),
        person.LastName(),
        convert_date(person.BirthDate()).c_str());
}

void event_planner::show_persons()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    printf("Persons Table\n");
    printf("--------\n");
    for (auto p : Persons_t::list())
    {
        show_person(p);
        printf("\n");
    }
    printf("--------\n");
}

void event_planner::delete_persons()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    for (auto person = Persons_t::get_first() ; person ; person = Persons_t::get_first())
    {
        person.delete_row();
    }
    tx.commit();
}

void event_planner::show_student(Students_t& student)
{
        auto p = student.Persons();
        printf("%lu: %u [Person]{", student.gaia_id(), student.Number());
        show_person(p);
        printf("}");
}

void event_planner::show_students()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    printf("Students Table\n");
    printf("--------\n");
    for (auto s : Students_t::list())
    {
        show_student(s);
        printf("\n");
    }
    printf("--------\n");
}

void event_planner::delete_students()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    for (auto s = Students_t::get_first() ; s; s = Students_t::get_first())
    {
        // Disconnect from Persons
        auto person = s.Persons();
        person.Students_list().erase(s);
        s.delete_row();
    }
    tx.commit();
}

void event_planner::show_staff_t(Staff_t& staff)
{
    auto p = staff.Persons();
    printf("%lu: %s [Person]{", staff.gaia_id(), convert_date(staff.HiredDate()).c_str());
    show_person(p);
    printf("}");
}

void event_planner::show_staff()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    printf("Staff Table\n");
    printf("--------\n");
    for (auto s : Staff_t::list())
    {
        show_staff_t(s);
        printf("\n");
    }
    printf("--------\n");
}

void event_planner::delete_staff()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    for (auto s = Staff_t::get_first() ; s; s = Staff_t::get_first())
    {
        // Disconnect from Persons
        s.Persons().Staff_list().erase(s);
        s.delete_row();
    }
    tx.commit();
}

void event_planner::show_parents()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    printf("Parents Table\n");
    printf("--------\n");
    for (auto parent : Parents_t::list())
    {
        auto p = parent.Persons();
        printf("%lu: %s [Person]{", parent.gaia_id(), parent.FatherOrMother());
        show_person(p);
        printf("}\n");
    }
    printf("--------\n");
}

void event_planner::delete_parents()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    for (auto p = Parents_t::get_first() ; p; p = Parents_t::get_first())
    {
        // Disconnect from Persons
        p.Persons().Parents_list().erase(p);
        p.delete_row();
    }
    tx.commit();
}

void event_planner::show_registrations()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    printf("Registrations Table\n");
    printf("--------\n");
    for (auto reg : Registrations_t::list())
    {
        auto s = reg.Student_Students();
        auto e = reg.Event_Events();
        printf("%lu: %s %s [Student]{", reg.gaia_id(),
            convert_date(reg.RegistrationDate()).c_str(),
            convert_time(reg.RegistrationTime()).c_str());
        show_student(s);
        printf("} [Event]{");
        show_event(e);
        printf("}\n");
    }
    printf("--------\n");
}

void event_planner::delete_registrations()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    for (auto r = Registrations_t::get_first() ; r; r = Registrations_t::get_first())
    {
        // Disconnect from Event
        r.Event_Events().Event_Registrations_list().erase(r);
        // Disconnect from Student
        r.Student_Students().Student_Registrations_list().erase(r);
        r.delete_row();
    }
    tx.commit();
}

void event_planner::show_event(Events_t& event)
{
    auto s = event.Teacher_Staff();
    auto r = event.Room_Rooms();
    printf("%lu: %s %s %s %s %u [Staff]{", event.gaia_id(),
        event.Name(),
        convert_date(event.Date()).c_str(),
        convert_time(event.StartTime()).c_str(),
        convert_time(event.EndTime()).c_str(),
        event.Enrolled()
    );
    show_staff_t(s);
    printf("} [Room]{");
    show_room(r);
    printf("}");
}

void event_planner::show_events()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    printf("Events Table\n");
    printf("--------\n");
    for (auto event : Events_t::list())
    {
        show_event(event);
        printf("\n");
    }
    printf("--------\n");
}

void event_planner::delete_events()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    for (auto e = Events_t::get_first() ; e; e = Events_t::get_first())
    {
        // Disconnect from Staff
        e.Teacher_Staff().Teacher_Events_list().erase(e);
        
        // Disconnect from Room
        if (e.Room_Rooms())
        {
            e.Room_Rooms().Room_Events_list().erase(e);
        }

        e.delete_row();
    }
    tx.commit();
}

void event_planner::show_restrictions()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    printf("Restrictions Table\n");
    printf("--------\n");
    for (auto r : Restrictions_t::list())
    {
        auto c = r.Campus();
        printf("%lu: %u [Campus]{", r.gaia_id(), r.PercentFull());
        show_campus_t(c);
        printf("}\n");
    }
    printf("--------\n");
}

void event_planner::delete_restrictions()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    for (auto r = Restrictions_t::get_first() ; r; r = Restrictions_t::get_first())
    {
        //Disconnect from campus
        auto campus = r.Campus();
        campus.Restrictions_list().erase(r);
        r.delete_row();
    }
    tx.commit();
}

void event_planner::show_all()
{
    show_campus();
    show_restrictions();
    show_buildings();
    show_rooms();
    show_persons();
    show_students();
    show_parents();
    show_staff();
    show_events();
    show_registrations();
}

void event_planner::delete_all()
{
    delete_restrictions();
    delete_registrations();
    delete_events();
    delete_rooms();
    delete_buildings();
    delete_campus();
    delete_students();
    delete_parents();
    delete_staff();
    delete_persons();
}

void event_planner::update_restriction(uint8_t percent)
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    auto restrictions = Restrictions_t::get_first();
    if (restrictions)
    {
        auto w = restrictions.writer();
        w.PercentFull = percent;
        w.update_row();
        tx.commit();
    }
}
