////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <gaia/direct_access/auto_transaction.hpp>
#include <gaia/system.hpp>

#include "gaia_clinic.h"
#include "lesson_manager.hpp"

using namespace gaia::clinic;

using gaia::direct_access::auto_transaction_t;

template <class T_object>
void remove_all_rows()
{
    const bool force_disconnect_of_related_rows = true;

    for (auto obj_it = T_object::list().begin();
         obj_it != T_object::list().end();)
    {
        auto this_it = obj_it++;
        this_it->delete_row(force_disconnect_of_related_rows);
    }
}

// Clean up the database between lessons.
void clean_db()
{
    auto_transaction_t txn(auto_transaction_t::no_auto_restart);
    remove_all_rows<physician_t>();
    remove_all_rows<location_t>();
    remove_all_rows<resident_t>();
    txn.commit();
}

// Add physicians, residents, and locations.  If connect_residents_to_physicians
// is 'true', then assign residents to physicians. residents are always
// assigned to locations based on their location_id.
void setup_clinic(bool connect_residents_to_physicians = false)
{
    // Location IDs.
    const uint32_t location_ab_id = 1;
    const uint32_t location_cd_id = 2;
    const uint32_t location_rd_id = 3;
    const uint32_t location_ak_id = 4;
    const uint32_t location_jh_id = 5;
    const uint32_t location_nn_id = 6;

    auto_transaction_t txn(auto_transaction_t::no_auto_restart);

    // Add physicians, residents, and locations.
    physician_t dr_cox = physician_t::get(physician_t::insert_row("Dr. Cox", true));
    physician_t dr_dorian = physician_t::get(physician_t::insert_row("Dr. Dorian", true));
    physician_t dr_reid = physician_t::get(physician_t::insert_row("Dr. Reid", true));

    // Add residents.
    resident_t pat_ab = resident_t::get(resident_t::insert_row("Amos Burton", location_ab_id, 71, false, {}));
    resident_t pat_cd = resident_t::get(resident_t::insert_row("Camina Drummer", location_cd_id, 66, false, {}));
    resident_t pat_rd = resident_t::get(resident_t::insert_row("Roberta Draper", location_rd_id, 72, true, {}));
    resident_t pat_ak = resident_t::get(resident_t::insert_row("Alex Kamal", location_ak_id, 68, false, {}));
    resident_t pat_jh = resident_t::get(resident_t::insert_row("Jim Holden", location_jh_id, 73, false, {}));
    resident_t pat_nn = resident_t::get(resident_t::insert_row("Naomi Nagata", location_nn_id, 67, false, {}));

    // Add locations (1 for each resident).  Because we are using value-linked references we do not need
    // to explicitly connect them.
    location_t add_ab = location_t::get(location_t::insert_row(location_ab_id, "17 Cherry Tree Lane", "Seattle"));
    location_t add_cd = location_t::get(location_t::insert_row(location_cd_id, "350 Fifth Avenue", "New York"));
    location_t add_rd = location_t::get(location_t::insert_row(location_rd_id, "221b Baker St", "Chicago"));
    location_t add_ak = location_t::get(location_t::insert_row(location_ak_id, "1313 Mockingbird Lane", "Georgia"));
    location_t add_jh = location_t::get(location_t::insert_row(location_jh_id, "63 West Wallaby St", "Lansing"));
    location_t add_nn = location_t::get(location_t::insert_row(location_nn_id, "742 Evergreen Terrace", "Springfield"));

    if (connect_residents_to_physicians)
    {
        // Connect residents to physicians (each physician gets two residents) to show
        // explicit relationships (non value-linked).
        dr_cox.residents().connect(pat_ab);
        dr_cox.residents().connect(pat_cd);
        dr_dorian.residents().connect(pat_rd);
        dr_dorian.residents().connect(pat_ak);
        dr_reid.residents().connect(pat_jh);
        dr_reid.residents().connect(pat_nn);
    }

    txn.commit();
}

// The following functions make inserts and updates to trigger rules in the tutorial.
void insert_doctor(const char* doctor_name, uint8_t count_rules)
{
    example_t example("db: inserting a physician in the database", count_rules);

    physician_t::insert_row(doctor_name, true);

    // Note that the transaction must be committed for those changes to
    // be seen by the rules engine. Rules are only fired after the commit.
    // In the examples, the commit call is happening when the 'example_t'
    // class goes out of scope.  See example_t::~example_t() in lesson_manager.hpp.
}

void insert_patient(const char* patient_name, uint8_t count_rules)
{
    example_t example("db: inserting a resident in the database", count_rules);

    // Note that we only fire a rule when we add a physician.
    // We can add a resident as well and no rules are fired.
    resident_writer resident;
    resident.name = patient_name;
    resident.is_intern = false;
    resident.insert_row();
}

void update_resident_status(const char* patient_name, bool status, uint8_t count_rules)
{
    example_t example("db: updating resident's status", count_rules);

    resident_t resident = *(resident_t::list().where(resident_expr::name == patient_name).begin());
    resident_writer writer = resident.writer();
    writer.is_intern = status;
    writer.update_row();
}

void update_resident_results(const char* patient_name, std::vector<float> results, uint8_t count_rules)
{
    example_t example("db: updating resident's results", count_rules);

    resident_t resident = *(resident_t::list().where(resident_expr::name == patient_name).begin());
    resident_writer writer = resident.writer();
    writer.evaluation_results = results;
    writer.update_row();
}

void update_physician_status(const char* doctor_name, bool status, uint8_t count_rules)
{
    example_t example("db: updating physician's status", count_rules);

    physician_t dr_cox = *(physician_t::list().where(physician_expr::name == doctor_name).begin());
    physician_writer writer = dr_cox.writer();
    writer.is_attending = status;
    writer.update_row();
}

void update_physician_name(const char* old_name, const char* new_name, uint8_t count_rules)
{
    example_t example("db: changing physician's name", count_rules);

    physician_t dr_cox = *(physician_t::list().where(physician_expr::name == old_name).begin());
    physician_writer writer = dr_cox.writer();
    writer.name = new_name;
    writer.update_row();
}

void insert_physician_and_patient(const char* doctor_name, const char* patient_name, uint8_t count_rules)
{
    example_t example("db: inserting physician and resident", count_rules);
    physician_t::insert_row("Dr. Cox", true);
    resident_t::insert_row("Chrisjen Avasarala", 0, 68, true, {});
}

void insert_location(uint8_t count_rules)
{
    example_t example("db: inserting location", count_rules);
    location_t::insert_row(17, "1 End of the Bar", "Deadwood");
}

void change_location(uint8_t count_rules)
{
    example_t example("db: changing location street", count_rules);

    location_t location = *(location_t::list().where(location_expr::city == "Lansing").begin());
    location_writer writer = location.writer();
    writer.street = "62 West Wallaby St.";
    writer.update_row();
}

// The following functions are the "lessons" of the tutorial. They work by using the
// functions above to make changes to the data that fire the rules in the lesson.
// Each lesson corresponds to a separate ruleset in 'clinic.ruleset'.  See that
// file for step by step documentation on each rule that is fired.

void arrays()
{
    update_resident_results("Amos Burton", {76.50, 89.75}, 1);
    update_resident_status("Amos Burton", true, 2);
}

void basics()
{
    insert_doctor("Dr. House", 1);
    insert_patient("Emma", 1);
    update_resident_status("Emma", true, 5);
}

void connections()
{
    update_resident_status("Camina Drummer", true, 1);
    update_resident_status("Camina Drummer", false, 1);
    change_location(1);
}

void forward_chaining()
{
    insert_patient("Emma", 2);
}

void insert_delete()
{
    update_physician_status("Dr. Dorian", false, 1);
    update_physician_status("Dr. Dorian", true, 1);
    update_resident_status("Rule Resident", false, 1);
}

void interop()
{
    update_physician_status("Dr. Cox", false, 1);
    update_physician_status("Dr. Dorian", false, 1);
    insert_doctor("Dr. Kelso", 1);
    insert_patient("Chrisjen Avasarala", 1);
    // For the next rule, delete all the locations.
    {
        auto_transaction_t tx(auto_transaction_t::no_auto_restart);
        remove_all_rows<location_t>();
        tx.commit();
    }
    insert_location(1);
    update_resident_status("Alex Kamal", true, 1);
    update_resident_status("Alex Kamal", false, 1);
}

void navigation()
{
    update_resident_status("Jim Holden", true, 2);
    update_physician_status("Dr. Cox", false, 1);
    update_physician_status("Dr. Cox", true, 1);
    update_physician_name("Dr. Cox", "Dr. Kelso", 1);
    insert_physician_and_patient("Dr. Cox", "Chrisjen Avasarala", 2);
    insert_location(1);
    change_location(1);
}

void tags()
{
    update_physician_status("Dr. Reid", false, 1);
    update_physician_status("Dr. Reid", true, 1);
    update_physician_name("Dr. Reid", "Dr. DeclareUseTag", 1);
    update_physician_name("Dr. Cox", "Dr. MultiTags", 1);
    update_physician_name("Dr. Dorian", "Dr. NestedTags", 1);
}

int main(int argc, const char** argv)
{
    // All lessons can be run sequentially or they
    // can be run one at a time by specifying the lesson on the
    // command line.
    lesson_manager_t lesson_manager;

    const bool setup_clinic = true;
    const bool connect_residents_to_physicians = true;

    lesson_manager.add_lesson("01_basics", basics, "basics");
    lesson_manager.add_lesson("02_forward_chaining", forward_chaining, "forward_chaining");
    lesson_manager.add_lesson("03_navigation", navigation, "navigation", setup_clinic, connect_residents_to_physicians);
    lesson_manager.add_lesson("04_tags", tags, "tags", setup_clinic, connect_residents_to_physicians);
    lesson_manager.add_lesson("05_connections", connections, "connections", setup_clinic);
    lesson_manager.add_lesson("06_insert_delete", insert_delete, "insert_delete", setup_clinic, connect_residents_to_physicians);
    lesson_manager.add_lesson("07_interop", interop, "interop", setup_clinic, connect_residents_to_physicians);
    lesson_manager.add_lesson("08_arrays", arrays, "arrays", setup_clinic);

    // Load up a custom configuration file so that we don't
    // log rule statistics during the tutorial.
    gaia::system::initialize("./gaia_tutorial.conf");

    args_handler_t args_handler(lesson_manager);
    args_handler.parse_and_run(argc, argv);

    gaia::system::shutdown();
    return EXIT_SUCCESS;
}
