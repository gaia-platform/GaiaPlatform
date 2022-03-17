////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <gaia/direct_access/auto_transaction.hpp>
#include <gaia/system.hpp>

#include "gaia_hospital.h"
#include "lesson_manager.hpp"

using namespace gaia::hospital;

using gaia::direct_access::auto_transaction_t;

// Clean up the database between lessons.
void clean_db()
{
    auto_transaction_t txn(auto_transaction_t::no_auto_restart);
    const bool remove_related_rows = true;

    for (auto doctor_it = doctor_t::list().begin();
         doctor_it != doctor_t::list().end();)
    {
        auto next_doctor_it = doctor_it++;
        next_doctor_it->patients().clear();
        next_doctor_it->delete_row();
    }

    for (auto patient_it = patient_t::list().begin();
         patient_it != patient_t::list().end();)
    {
        auto next_patient_it = patient_it++;
        next_patient_it->delete_row(remove_related_rows);
    }
    for (auto address_it = address_t::list().begin();
         address_it != address_t::list().end();)
    {
        auto next_address_it = address_it++;
        next_address_it->delete_row();
    }

    txn.commit();
}

// Add doctors, patients, and addresses.  If connect_patients_to_doctors
// is 'true', then assign patients to doctors. Patients are always
// assigned to addresses based on their address_id.
void setup_clinic(bool connect_patients_to_doctors = false)
{
    // Address IDs
    const uint32_t address_ab_id = 1;
    const uint32_t address_cd_id = 2;
    const uint32_t address_rd_id = 3;
    const uint32_t address_ak_id = 4;
    const uint32_t address_jh_id = 5;
    const uint32_t address_nn_id = 6;

    auto_transaction_t txn(auto_transaction_t::no_auto_restart);

    // add doctors, patients, and addresses
    doctor_t dr_cox = doctor_t::get(doctor_t::insert_row("Dr. Cox", true));
    doctor_t dr_dorian = doctor_t::get(doctor_t::insert_row("Dr. Dorian", true));
    doctor_t dr_reid = doctor_t::get(doctor_t::insert_row("Dr. Reid", true));

    // Add patients
    patient_t pat_ab = patient_t::get(patient_t::insert_row("Amos Burton", address_ab_id, 71, false, {}));
    patient_t pat_cd = patient_t::get(patient_t::insert_row("Camina Drummer", address_cd_id, 66, false, {}));
    patient_t pat_rd = patient_t::get(patient_t::insert_row("Roberta Draper", address_rd_id, 72, false, {}));
    patient_t pat_ak = patient_t::get(patient_t::insert_row("Alex Kamal", address_ak_id, 68, false, {}));
    patient_t pat_jh = patient_t::get(patient_t::insert_row("Jim Holden", address_jh_id, 73, false, {}));
    patient_t pat_nn = patient_t::get(patient_t::insert_row("Naomi Nagata", address_nn_id, 67, false, {}));

    // Add addresses (1 for each patient).  Because we are using value-linked references we do not need
    // to explicitly connect them.
    address_t add_ab = address_t::get(address_t::insert_row(address_ab_id, "17 Cherry Tree Lane", "Seattle"));
    address_t add_cd = address_t::get(address_t::insert_row(address_cd_id, "350 Fifth Avenue", "New York"));
    address_t add_rd = address_t::get(address_t::insert_row(address_rd_id, "221b Baker St", "Chicago"));
    address_t add_ak = address_t::get(address_t::insert_row(address_ak_id, "1313 Mockingbird Lane", "Georgia"));
    address_t add_jh = address_t::get(address_t::insert_row(address_jh_id, "63 West Wallaby St", "Lansing"));
    address_t add_nn = address_t::get(address_t::insert_row(address_nn_id, "742 Evergreen Terrace", "Springfield"));

    if (connect_patients_to_doctors)
    {
        // Connect patients to doctors (each doctor gets two patients) to show
        // explicit relationships (non value-linked).
        dr_cox.patients().connect(pat_ab);
        dr_cox.patients().connect(pat_cd);
        dr_dorian.patients().connect(pat_rd);
        dr_dorian.patients().connect(pat_ak);
        dr_reid.patients().connect(pat_jh);
        dr_reid.patients().connect(pat_nn);
    }

    txn.commit();
}

// The follwing functions make inserts and updates to trigger rules in the tutorial.
void insert_doctor(const char* doctor_name, uint8_t count_rules)
{
    example_t example("db: inserting a doctor in the database", count_rules);

    doctor_t::insert_row(doctor_name, true);

    // Note that the transaction must be committed for those changes to
    // be seen by the rules engine. Rules are only fired after the commit.
    // In the examples, the commit call is happening when the 'example_t'
    // class goes out of scope.  See example_t::~example_t() in lesson_manager.hpp.
}

void insert_patient(const char* patient_name, uint8_t count_rules)
{
    example_t example("db: inserting a patient in the database", count_rules);

    // Note that we only fire a rule when we add a doctor.
    // We can add a patient as well and no rules are fired.
    patient_writer patient;
    patient.name = patient_name;
    patient.is_active = false;
    patient.insert_row();
}

void update_patient_status(const char* patient_name, bool status, uint8_t count_rules)
{
    // In order to reduce boilerplate code, all examples will wrap
    // transaction handling in the example_t class constructor and
    // destructor from now on.
    example_t example("db: updating patient's status", count_rules);

    patient_t patient = *(patient_t::list().where(patient_expr::name == patient_name).begin());
    patient_writer writer = patient.writer();
    writer.is_active = status;
    writer.update_row();
}

void update_patient_results(const char* patient_name, std::vector<float> results, uint8_t count_rules)
{
    // In order to reduce boilerplate code, all examples will wrap
    // transaction handling in the example_t class constructor and
    // destructor from now on.
    example_t example("db: updating patient's results", count_rules);

    // Retrieve the first (and only) patient
    patient_t patient = *(patient_t::list().where(patient_expr::name == patient_name).begin());
    patient_writer writer = patient.writer();
    writer.analysis_results = results;
    writer.update_row();
}

void update_doctor_status(const char* doctor_name, bool status, uint8_t count_rules)
{
    example_t example("db: updating doctor's active status", count_rules);

    doctor_t dr_cox = *(doctor_t::list().where(doctor_expr::name == doctor_name).begin());
    doctor_writer writer = dr_cox.writer();
    writer.is_active = status;
    writer.update_row();
}

void update_doctor_name(const char* old_name, const char* new_name, uint8_t count_rules)
{
    example_t example("db: changing doctor's name", count_rules);

    doctor_t dr_cox = *(doctor_t::list().where(doctor_expr::name == old_name).begin());
    doctor_writer writer = dr_cox.writer();
    writer.name = new_name;
    writer.update_row();
}

void insert_doctor_and_patient(const char* doctor_name, const char* patient_name, uint8_t count_rules)
{
    example_t example("db: inserting doctor and patient", count_rules);
    doctor_t::insert_row("Dr. Cox", true);
    patient_t::insert_row("Chrisjen Avasarala", 0, 68, true, {});
}

void insert_address(uint8_t count_rules)
{
    example_t example("db: inserting address", count_rules);
    address_t::insert_row(17, "1 End of the Bar", "Deadwood");
}

void change_address(uint8_t count_rules)
{
    example_t example("db: changing address street", count_rules);

    address_t address = *(address_t::list().where(address_expr::city == "Lansing").begin());
    address_writer writer = address.writer();
    writer.street = "62 West Wallaby St.";
    writer.update_row();
}

// The following functions are the "lessons" of the tutorial. They work by using the
// functions above to make changes to the data that fire the rules in the lesson.
// Each lesson corresponds to a separate ruleset in 'hospital.ruleset'.  See that
// file for step by step documentation on each rule that is fired.

void arrays()
{
    update_patient_results("Amos Burton", {1.23, 4.56}, 1);
    update_patient_status("Amos Burton", true, 2);
}

void basics()
{
    insert_doctor("Dr. House", 1);
    insert_patient("Emma", 1);
    update_patient_status("Emma", true, 5);
}

void connections()
{
    update_patient_status("Camina Drummer", true, 1);
    update_patient_status("Camina Drummer", false, 1);
    change_address(1);
}

void forward_chaining()
{
    insert_patient("Emma", 2);
}

void insert_delete()
{
    update_doctor_status("Dr. Dorian", false, 1);
    update_doctor_status("Dr. Dorian", true, 1);
    update_patient_status("Rule Patient", false, 1);
}

void interop()
{
    update_doctor_status("Dr. Cox", false, 1);
    update_doctor_status("Dr. Dorian", false, 1);
}

void navigation()
{
    update_patient_status("Jim Holden", true, 1);
    update_patient_status("Jim Holden", false, 1);
    update_doctor_status("Dr. Cox", false, 1);
    update_doctor_status("Dr. Cox", true, 1);
    update_doctor_name("Dr. Cox", "Dr. Kelso", 1);
    insert_doctor_and_patient("Dr. Cox", "Chrisjen Avasarala", 2);
    insert_address(1);
}

void tags()
{
    update_doctor_status("Dr. Reid", false, 1);
    update_doctor_status("Dr. Reid", true, 1);
    update_doctor_name("Dr. Reid", "Dr. DeclareUseTag", 1);
    update_doctor_name("Dr. Cox", "Dr. MultiTags", 1);
    update_doctor_name("Dr. Dorian", "Dr. NestedTags", 1);
}

int main(int argc, const char** argv)
{
    // All lessons can be run sequentially or they
    // can be run one at a time by specifying the lesson on the
    // command line.
    lesson_manager_t lesson_manager;

    const bool setup_clinic = true;
    const bool connect_patients_to_doctors = true;

    lesson_manager.add_lesson("01_basics", basics, "basics");
    lesson_manager.add_lesson("02_forward_chaining", forward_chaining, "forward_chaining");
    lesson_manager.add_lesson("03_navigation", navigation, "navigation", setup_clinic, connect_patients_to_doctors);
    lesson_manager.add_lesson("04_tags", tags, "tags", setup_clinic, connect_patients_to_doctors);
    lesson_manager.add_lesson("05_connections", connections, "connections", setup_clinic);
    lesson_manager.add_lesson("06_insert_delete", insert_delete, "insert_delete", setup_clinic, connect_patients_to_doctors);
    lesson_manager.add_lesson("07_interop", interop, "interop", setup_clinic, connect_patients_to_doctors);
    lesson_manager.add_lesson("08_arrays", arrays, "arrays", setup_clinic);

    // Load up a custom configuration file so that we don't
    // log rule statistics during the tutorial.
    gaia::system::initialize("./gaia_tutorial.conf");

    args_handler_t args_handler(lesson_manager);
    args_handler.parse_and_run(argc, argv);

    gaia::system::shutdown();
    return EXIT_SUCCESS;
}
