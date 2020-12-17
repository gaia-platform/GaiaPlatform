#include <atomic>
#include <filesystem>
#include <iostream>

#include <cli/cli.h>
#include <cli/clilocalsession.h>
#include <cli/filehistorystorage.h>

#include <gaia/common.hpp>
#include <gaia/db/db.hpp>
#include <gaia/direct_access/auto_transaction.hpp>
#include <gaia/stream/Stream.h>
#include <gaia/stream/StreamOperators.h>
#include <gaia/system.hpp>
#include "crud.h"
#include "gaia_school.h"
#include "logger.h"
#include "utils.h"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::direct_access;

using namespace stream::op;
using namespace cli;
using namespace std;
using namespace chrono;
using namespace gaia::school;

namespace fs = filesystem;

void menu()
{
#if BOOST_VERSION < 106600
    boost::asio::io_service ios;
#else
    boost::asio::io_context ios;
#endif

    // setup cli

    auto root_menu = make_unique<Menu>("cli");

    root_menu->Insert(
        "list_students",
        [](ostream& out) {
            auto_transaction_t txn;
            for (auto student : students_t::list())
            {
                out << print_person(student.student_person_persons()) << ' '
                    << student.telephone() << endl;
            }
            txn.commit();
        },
        "List students");

    root_menu->Insert(
        "list_parents",
        [](ostream& out) {
            auto_transaction_t txn;
            for (auto parent : parents_t::list())
            {
                out << print_person(parent.parent_person_persons()) << ' '
                    << parent.parent_type() << endl;
            }
            txn.commit();
        },
        "List parents");

    root_menu->Insert(
        "list_staff",
        [](ostream& out) {
            auto_transaction_t txn;
            for (auto staff : staff_t::list())
            {
                out << print_person(staff.person_persons()) << endl;
            }
            txn.commit();
        },
        "List staff");

    root_menu->Insert(
        "list_families",
        [](ostream& out) {
            auto_transaction_t txn;
            for (auto family : family_t::list())
            {
                out << "Father: " << print_person(family.father_parents().parent_person_persons()) << endl
                    << "Mother: " << print_person(family.mother_parents().parent_person_persons()) << endl
                    << "Child: " << print_person(family.student_students().student_person_persons()) << endl
                    << endl;
            }
            txn.commit();
        },
        "List families");

    root_menu->Insert(
        "list_buildings",
        [](ostream& out) {
            auto_transaction_t txn;
            for (auto building : buildings_t::list())
            {
                out << building.building_name() << endl;

                for (const auto& room : building.building_rooms_list())
                {
                    out << "  " << print_room(room) << endl;
                }
                out << endl;
            }
            txn.commit();
        },
        "List buildings");

    root_menu->Insert(
        "list_events",
        [](ostream& out) {
            auto_transaction_t txn;
            for (const auto& event : events_t::list())
            {

                out << print_event(event) << endl;
            }
            txn.commit();
        },
        "List events");

    root_menu->Insert(
        "event_details", {"event_name"},
        [](ostream& out, const std::string& event_name) {
            auto_transaction_t txn;
            try
            {
                events_t event = events_t::stream()
                    | filter(events_t::fn::name == event_name)
                    | first();

                out << print_event(event) << endl;

                out << "Attendees: " << endl;

                event.event_registration_list().stream()
                    | map_(registration_t::fn::reg_student_students)
                    | for_each([&](students_t student) {
                          out << "  " << print_person(student.student_person_persons()) << endl;
                      });
            }
            catch (stream::EmptyStreamException&)
            {
                out << "Event " << event_name << " does not exist" << endl;
                return;
            }
            txn.commit();
        },
        "Event details");

    root_menu->Insert(
        "register_to_event", {"event_name", "first_name", "last_name"},
        [](ostream& out, const string& event_name, const string& name, const string& surname) {
            auto_transaction_t txn;
            events_t event;
            try
            {
                event = events_t::stream()
                    | filter(events_t::fn::name == event_name)
                    | first();
            }
            catch (stream::EmptyStreamException&)
            {
                out << "Event " << event_name << " does not exist" << endl;
                return;
            }

            students_t student;
            try
            {
                student = students_t::stream()
                    | filter([&](students_t& s) {
                              return iequals(s.student_person_persons().first_name(), name.c_str())
                                  && iequals(s.student_person_persons().last_name(), surname.c_str());
                          })
                    | first();
            }
            catch (stream::EmptyStreamException&)
            {
                out << "Student " << name << " " << surname << " does not exist" << endl;
                return;
            }

            auto registration = registration_t::get(
                registration_t::insert_row(current_time_millis()));

            student.reg_student_registration_list().insert(registration);
            event.event_registration_list().insert(registration);

            txn.commit();
            sleep(1);
        },
        "Register to event");

    root_menu->Insert(
        "enter_building", {"building", "name", "surname"},
        [](ostream& out, const string& building_name, const string& name, const string& surname) {
            auto_transaction_t txn;

            gaia_id_t scan_log = face_scan_log_t::insert_row(
                create_face_signature(name, surname),
                current_time_millis(), person_type::undefined);

            buildings_t building = buildings_t::stream()
                | filter(buildings_t::fn::building_name == building_name)
                | first();

            building.building_log_face_scan_log_list().insert(scan_log);

            txn.commit();
            sleep(1);
        },
        "Enter building");

    // create a cli with the given root menu and a persistent storage
    // you must pass to FileHistoryStorage the path of the history file
    // if you don't pass the second argument, the cli will use a VolatileHistoryStorage object that keeps in memory
    // the history of all the sessions, until the cli is shut down.
    Cli cli(move(root_menu), make_unique<FileHistoryStorage>(".cli"));
    // global exit action
    cli.ExitAction([](auto& out) { out << "Goodbye and thanks for all the fish.\n"; });
    // std exception custom handler
    cli.StdExceptionHandler(
        [](ostream& out, const string& cmd, const exception& e) {
            out << "Exception caught in cli handler: \n \""
                << e.what()
                << "\"\n handling command: "
                << cmd
                << ".\n";
        });

    CliLocalTerminalSession local_session(cli, ios, cout, 200);
    local_session.ExitAction(
        [&ios](auto& out) // session exit action
        {
            out << "Closing App...\n";
            ios.stop();
        });

    // setup server

#if BOOST_VERSION < 106600
    boost::asio::io_service::work work(ios);
#else
    auto work = boost::asio::make_work_guard(ios);
#endif

    ios.run();
}

log_conf_t g_log_conf = {};

int main(int argc, char** argv)
{
    gaia::system::initialize("conf/gaia.conf");
    g_log_conf.headers = true;
    g_log_conf.level = debug;

    begin_transaction();
    init_data();
    commit_transaction();

    menu();

    return 0;
}
