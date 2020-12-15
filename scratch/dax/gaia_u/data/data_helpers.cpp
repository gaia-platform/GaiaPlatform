
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

string event_planner::convert_time(int64_t timestamp)
{
    char buffer[12];
    tm* t_ptr = localtime(&timestamp);
    strftime(buffer, sizeof(buffer), event_planner::c_time_format, t_ptr);
    return string(buffer);
}


void event_planner::show_building(Buildings_t& building)
{
    printf("%lu: %s", building.gaia_id(), building.BuildingName());
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
        building.delete_row();
    }
    tx.commit();
}

void event_planner::show_room(Rooms_t& room)
{
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
    printf("%lu: %s, %s, %s",
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

void event_planner::show_students()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    printf("Students Table\n");
    printf("--------\n");
    for (auto s : Students_t::list())
    {
        auto p = s.Persons();
        printf("%lu: %u [Person]{", s.gaia_id(), s.Number());
        show_person(p);
        printf("}\n");
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

void event_planner::show_events()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    printf("Events Table\n");
    printf("--------\n");
    for (auto event : Events_t::list())
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
        printf("}\n");
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
        e.Room_Rooms().Room_Events_list().erase(e);

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
        printf("%lu: %u\n", r.gaia_id(), r.PercentFull());
    }
    printf("--------\n");
}

void event_planner::delete_restrictions()
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    for (auto r = Restrictions_t::get_first() ; r; r = Restrictions_t::get_first())
    {
        r.delete_row();
    }
    tx.commit();
}

void event_planner::show_all()
{
    show_restrictions();
    show_buildings();
    show_rooms();
    show_persons();
    show_students();
    show_parents();
    show_staff();
    show_events();
}

void event_planner::delete_all()
{
    delete_restrictions();
    delete_events();
    delete_rooms();
    delete_buildings();
    delete_students();
    delete_parents();
    delete_staff();
    delete_persons();
}

void event_planner::update_restriction(uint8_t percent)
{
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    auto w = Restrictions_t::get_first().writer();
    w.PercentFull = percent;
    w.update_row();
    tx.commit();
}
