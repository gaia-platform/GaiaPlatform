
#include "gaia_gaia_u.h"
#include "utils.hpp"
#include "loader.hpp"
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

// Agree with Simone, shouldn't have to provide this.
extern "C" void initialize_rules()
{
}

/*
uint32_t remove_events()
{
    uint32_t count = 0;
    auto_transaction_t tx(auto_transaction_t::no_auto_begin);
    Events_t event = Events_t::get_first();
    while (event)
    {
        event.delete_row();
        event = Events_t::get_first();
        count++;
    }
    tx.commit();
    return count;
}
*/

void show_building(Buildings_t& building)
{
    printf("%lu: %s", building.gaia_id(), building.BuildingName());
}

void show_buildings()
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

void show_room(Rooms_t& room)
{
    auto b = room.Buildings();
    printf("%lu: %u, %s, %u, %u [Building]{",
        room.gaia_id(), room.RoomNumber(), room.RoomName(), room.FloorNumber(), room.Capacity());
    show_building(b);
    printf("}");
}

void show_rooms()
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

void show_person(Persons_t& person)
{
    printf("%lu: %s, %s, %s",
        person.gaia_id(),
        person.FirstName(),
        person.LastName(),
        utils_t::convert_date(person.BirthDate()).c_str());
}

void show_persons()
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

void show_students()
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

void show_staff_t(Staff_t& staff)
{
    auto p = staff.Persons();
    printf("%lu: %s [Person]{", staff.gaia_id(), utils_t::convert_date(staff.HiredDate()).c_str());
    show_person(p);
    printf("}");
}

void show_staff()
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

void show_parents()
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

void show_events()
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
            utils_t::convert_date(event.Date()).c_str(),
            utils_t::convert_time(event.StartTime()).c_str(),
            utils_t::convert_time(event.EndTime()).c_str(),
            event.Enrolled()
        );
        show_staff_t(s);
        printf("} [Room]{");
        show_room(r);
        printf("}\n");
    }
    printf("--------\n");
}


void usage(const char*command) 
{
    printf("Usage: %s [filename]\n", command);
}

int main(int argc, const char**argv) {
    printf("-----------------------------------------\n");
    printf("Gaia U Data Loader\n");
    printf("-----------------------------------------\n");
    gaia::system::initialize();
    if (argc == 1)
    {
        // Dump the database.
        show_buildings();
        show_rooms();
        show_persons();
        show_students();
        show_parents();
        show_staff();
        show_events();
    }
    else
    if (argc == 2)
    {
        // Load the database.
        gaia_u_loader_t loader;
        loader.load(argv[1]);

    }
    else
    {
        printf("Incorrect arguments\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    gaia::system::shutdown();
}
