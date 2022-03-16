////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <unistd.h>

#include <iostream>

#include <gaia/db/db.hpp>
#include <gaia/exceptions.hpp>
#include <gaia/logger.hpp>
#include <gaia/rules/rules.hpp>
#include <gaia/system.hpp>

#include "gaia_hospital.h"

using namespace gaia::hospital;
using namespace gaia::rules;
using gaia::common::gaia_id_t;
using gaia::direct_access::auto_transaction_t;

std::string g_lesson;

/**
 * Clean the database.
 */
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

const uint32_t c_wait_time = 10000; // 10 milliseconds
class example_t
{
public:
    example_t(const char* description)
    {
        gaia_log::app().info(description);
    }

    ~example_t()
    {
        m_transaction.commit();
        // For example purposes only, wait for the rule to be complete.
        usleep(c_wait_time);
        gaia_log::app().info("Press <enter> to continue...");
        std::cin.get();
    }

private:
    auto_transaction_t m_transaction;
};

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

void insert_doctor(const char* doctor_name)
{
    example_t example("db: inserting a doctor in the database");

    doctor_t::insert_row(doctor_name, true);

    // Note that the transaction must be committed for those changes to
    // be seen by the rules engine. Rules are only fired after the commit.
    // In the examples, the commit call is happening when the 'example_t'
    // class goes out of scope.
}

void insert_patient(const char* patient_name)
{
    example_t example("db: inserting a patient in the database");

    // Note that we only fire a rule when we add a doctor.
    // We can add a patient as well and no rules are fired.
    patient_writer patient;
    patient.name = patient_name;
    patient.is_active = false;
    patient.insert_row();
}

void update_patient_status(const char* patient_name, bool status)
{
    // In order to reduce boilerplate code, all examples will wrap
    // transaction handling in the example_t class constructor and
    // destructor from now on.
    example_t example("db: updating patient's status");

    // Retrieve the first (and only) patient
    patient_t patient = *(patient_t::list().where(patient_expr::name == patient_name).begin());
    patient_writer writer = patient.writer();
    writer.is_active = status;
    writer.update_row();
}

void update_patient_results(const char* patient_name, std::vector<float> results)
{
    // In order to reduce boilerplate code, all examples will wrap
    // transaction handling in the example_t class constructor and
    // destructor from now on.
    example_t example("db: updating patient's results");

    // Retrieve the first (and only) patient
    patient_t patient = *(patient_t::list().where(patient_expr::name == patient_name).begin());
    patient_writer writer = patient.writer();
    writer.analysis_results = results;
    writer.update_row();
}

void update_doctor_status(const char* doctor_name, bool status)
{
    example_t example("db: updating doctor's active status");

    doctor_t dr_cox = *(doctor_t::list().where(doctor_expr::name == doctor_name).begin());
    doctor_writer writer = dr_cox.writer();
    writer.is_active = status;
    writer.update_row();
}

void update_doctor_name(const char* old_name, const char* new_name)
{
    example_t example("db: changing doctor's name");

    // Make Dr. Cox inactive.
    doctor_t dr_cox = *(doctor_t::list().where(doctor_expr::name == old_name).begin());
    doctor_writer writer = dr_cox.writer();
    writer.name = new_name;
    writer.update_row();
}

void insert_doctor_and_patient(const char* doctor_name, const char* patient_name)
{
    example_t example("db: inserting doctor and patient");
    doctor_t::insert_row("Dr. Cox", true);
    patient_t::insert_row("Chrisjen Avasarala", 0, 68, true, {});
}

void insert_address()
{
    example_t example("db: inserting address");
    address_t::insert_row(17, "1 End of the Bar", "Deadwood");
}

void change_address()
{
    example_t example("db: changing address street");

    // Move Jim to the same city as Naomi.
    address_t address = *(address_t::list().where(address_expr::city == "Lansing").begin());
    address_writer writer = address.writer();
    writer.street = "62 West Wallaby St.";
    writer.update_row();
}

bool run_lesson(const char* name)
{

    if (g_lesson.empty() || g_lesson == name)
    {
        unsubscribe_rules();
        clean_db();

        std::string lesson(name);

        if ((lesson == "navigation")
            || (lesson == "tags")
            || (lesson == "interop")
            || (lesson == "insert_delete")
            || (lesson == "arrays"))
        {
            const bool connect_patients_to_doctors = true;
            setup_clinic(connect_patients_to_doctors);
        }
        else if ((lesson == "connections"))
        {
            setup_clinic();
        }
        subscribe_ruleset(name);
        gaia_log::app().info("--- [Lesson {}] ---", name);
        return true;
    }

    return false;
}

int main(int argc, const char** argv)
{
    // It is helpful to walk through the direct_access sample before running this one to get
    // acquainted with how to interact with the Gaia database.  This tutorial focuses on
    // the syntax for rules.  Rules are fired in response to changes made to the database.

    // Load up a custom configuration file so that we don't
    // log rule statistics during the tutorial.
    gaia::system::initialize("./gaia_tutorial.conf");

    // By default, all rulesets are subscribed and active after initialization.
    // For this tutorial we'll only subscribe one ruleset at a time per lesson.
    unsubscribe_rules();

    if (argc == 2)
    {
        g_lesson = argv[1];
    }

    if (run_lesson("basics"))
    {
        insert_doctor("Dr. House");
        insert_patient("Emma");
        update_patient_status("Emma", true);
    }

    if (run_lesson("forward_chaining"))
    {
        insert_patient("Emma");
    }

    if (run_lesson("navigation"))
    {
        update_patient_status("Jim Holden", true);
        update_patient_status("Jim Holden", false);
        update_doctor_status("Dr. Cox", false);
        update_doctor_status("Dr. Cox", true);
        update_doctor_name("Dr. Cox", "Dr. Kelso");
        insert_doctor_and_patient("Dr. Cox", "Chrisjen Avasarala");
        insert_address();
    }

    if (run_lesson("tags"))
    {
        update_doctor_status("Dr. Reid", false);
        update_doctor_status("Dr. Reid", true);
        update_doctor_name("Dr. Reid", "Dr. DeclareUseTag");
        update_doctor_name("Dr. Cox", "Dr. MultiTags");
        update_doctor_name("Dr. Dorian", "Dr. NestedTags");
    }

    if (run_lesson("insert_delete"))
    {
        update_doctor_status("Dr. Dorian", false);
        update_doctor_status("Dr. Dorian", true);
        update_patient_status("Rule Patient", false);
    }

    if (run_lesson("connections"))
    {
        update_patient_status("Camina Drummer", true);
        update_patient_status("Camina Drummer", false);
        change_address();
    }

    if (run_lesson("interop"))
    {
        update_doctor_status("Dr. Cox", false);
        update_doctor_status("Dr. Dorian", false);
    }

    if (run_lesson("arrays"))
    {
        update_patient_results("Amos Burton", {1.23, 4.56});
        update_patient_status("Amos Burton", true);
    }

    /*
        create_record_insert();
        create_record_writer();
        lookup_record_get();
        array_type_fields();
        update_record();
        optional_values();
        lookup_invalid_record();
        access_invalid_record();
        compare_records();
        list_all_patients();
        clean_db();
        delete_single_record();
        delete_single_record_static();
        delete_all_records();
        traverse_one_to_many_relationship();
        clean_db();
        delete_one_to_many_relationship_re();
        clean_db();
        delete_one_to_many_relationship();
        clean_db();
        delete_one_to_many_relationship_erase();
        clean_db();
        traverse_one_to_one_relationship();
        clean_db();
        delete_one_to_one_relationship();
        clean_db();
        create_filter_data();
        filter_lambda();
        filter_gaia_predicates_strings();
        filter_gaia_predicates_numbers();
        filter_gaia_predicates_containers();
        clean_db();

        txn.commit();

        use_dac_object_across_transactions();
    */
    gaia::system::shutdown();
}
