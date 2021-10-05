/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gaia/db/db.hpp>
#include <gaia/logger.hpp>
#include <gaia/system.hpp>

#include "gaia_hospital.h"

using namespace gaia::hospital;
using namespace gaia::common;

void clean_db()
{

    for (auto& doctor = *doctor_t::list().begin();
         doctor; doctor = *doctor_t::list().begin())
    {
        doctor.patients().clear();
        doctor.delete_row();
    }

    for (auto& patient = *patient_t::list().begin();
         patient; patient = *patient_t::list().begin())
    {
        patient.address().disconnect();
        patient.delete_row();
    }

    for (auto& address = *address_t::list().begin();
         address; address = *address_t::list().begin())
    {
        address.delete_row();
    }
}

void create_record_insert()
{
    // Inserts a single record
    [[maybe_unused]] gaia_id_t id = patient_t::insert_row("Jane", 183, true);
}

void create_record_writer()
{
    patient_writer patient_w;
    patient_w.name = "Emma";
    patient_w.is_female = true;
    [[maybe_unused]] gaia_id_t id = patient_w.insert_row();
}

void lookup_record_get()
{
    gaia_id_t id = patient_t::insert_row("John", 175, false);

    patient_t john = patient_t::get(id);
    gaia_log::app().info("Patient name: {}", john.name());
}

void lookup_record_get_not_existent()
{
    gaia_id_t invalid_id = 12344;
    patient_t john = patient_t::get(invalid_id);

    if (john)
    {
        gaia_log::app().info("Patient name: {}", john.name());
    }
    else
    {
        gaia_log::app().warn("Cannot find patient with id: {}", invalid_id);
    }
}

void access_invalid_record()
{
    gaia_id_t invalid_id = 12344;
    patient_t john = patient_t::get(invalid_id);

    try
    {
        gaia_log::app().info("Patient name: {}", john.name());
    }
    catch (gaia::db::invalid_object_id& ex)
    {
        gaia_log::app().error("An exception occurred while accessing a record: '{}'.", ex.what());
    }
}

void iterate_all_patients()
{
    for (auto& patient : patient_t::list())
    {
        gaia_log::app().info(
            "Patient name:{}, height:{}", patient.name(), patient.height());
    }
}

void delete_single_record()
{
    doctor_t house = doctor_t::get(doctor_t::insert_row("Dr. House"));

    house.delete_row();

    if (!house)
    {
        gaia_log::app().info("The record has been deleted");
    }
}

void delete_all_records()
{
    doctor_t house = doctor_t::get(doctor_t::insert_row("Dr. House"));
    doctor_t dorian = doctor_t::get(doctor_t::insert_row("Dr. Dorian"));
    doctor_t reid = doctor_t::get(doctor_t::insert_row("Dr. Reid"));

    for (auto& doctor = *doctor_t::list().begin();
         doctor; doctor = *doctor_t::list().begin())
    {
        doctor.delete_row();
    }

    gaia_log::app().info("Num doctors: {}", doctor_t::list().size());
}

gaia_id_t create_one_to_many_relationship()
{
    doctor_t house = doctor_t::get(doctor_t::insert_row("Dr. House"));

    patient_t jane = patient_t::get(patient_t::insert_row("Jane", 183, true));
    patient_t jack = patient_t::get(patient_t::insert_row("Jack", 176, false));
    gaia_id_t john_id = patient_t::insert_row("John", 175, false);

    // Type safe insert.
    house.patients().insert(jane);
    house.patients().insert(jack);

    // Type unsafe insert.
    house.patients().insert(john_id);

    return house.gaia_id();
}

void traverse_one_to_many_relationship(gaia_id_t doctor_id)
{
    doctor_t house = doctor_t::get(doctor_id);

    for (auto& patient : house.patients())
    {
        gaia_log::app().info("Patient name: {}", patient.name());
        gaia_log::app().info("Patient's doctor: {}", patient.doctor().name());
    }
}

void delete_one_to_many_relationship_re(gaia_id_t doctor_id)
{
    doctor_t house = doctor_t::get(doctor_id);

    // Pick one of the doctor's patients.
    patient_t patient = *house.patients().begin();

    try
    {
        house.delete_row();
    }
    catch (const gaia::db::object_still_referenced& ex)
    {
        gaia_log::app().error("An exception occurred while deleting doctor record '{}'.", ex.what());
    }

    try
    {
        patient.delete_row();
    }
    catch (const gaia::db::object_still_referenced& ex)
    {
        gaia_log::app().error("An exception occurred while deleting patient record '{}'.", ex.what());
    }
}

void delete_one_to_many_relationship(gaia_id_t doctor_id)
{
    doctor_t house = doctor_t::get(doctor_id);

    // Pick one of the doctor's patients.
    patient_t patient = *house.patients().begin();

    // You can unlink a single element (there are still 2 connected).
    house.patients().remove(patient);

    // You can now delete the patient.
    patient.delete_row();

    // Unlink all the rem remaining patients.
    house.patients().clear();

    // You can now delete the doctor.
    house.delete_row();
}

gaia_id_t create_one_to_one_relationship()
{
    patient_t jane = patient_t::get(patient_t::insert_row("Jane", 183, true));
    address_t kissimmee = address_t::get(address_t::insert_row("Hamlet Ln", "Kissimmee"));

    jane.address().connect(kissimmee);

    return jane.gaia_id();
}

void traverse_one_to_one_relationship(gaia_id_t patient_id)
{
    patient_t patient = patient_t::get(patient_id);
    // Actually returns address_ref_t.
    address_t address = patient.address(); // NOLINT(cppcoreguidelines-slicing)

    gaia_log::app().info("City {}", address.city());
    gaia_log::app().info("Patient {}", address.patient().name());
}

void delete_one_to_one_relationship_re(gaia_id_t patient_id)
{
    patient_t patient = patient_t::get(patient_id);
    address_t address = patient.address(); // NOLINT(cppcoreguidelines-slicing)

    try
    {
        patient.delete_row();
    }
    catch (const gaia::db::object_still_referenced& ex)
    {
        gaia_log::app().error("An exception occurred while deleting a patient record '{}'.", ex.what());
    }

    try
    {
        address.delete_row();
    }
    catch (const gaia::db::object_still_referenced& ex)
    {
        gaia_log::app().error("An exception occurred while deleting an address record '{}'.", ex.what());
    }
}

void delete_one_to_one_relationship(gaia_id_t patient_id)
{
    patient_t patient = patient_t::get(patient_id);
    address_t address = patient.address(); // NOLINT(cppcoreguidelines-slicing)

    patient.address().disconnect();

    patient.delete_row();
    address.delete_row();
}

void create_filter_data()
{
    doctor_t dorian = doctor_t::get(doctor_t::insert_row("Dr. Dorian"));
    doctor_t house = doctor_t::get(doctor_t::insert_row("Dr. House"));

    patient_t jane = patient_t::get(patient_t::insert_row("Jane", 183, true));
    patient_t jack = patient_t::get(patient_t::insert_row("Jack", 176, false));
    patient_t john = patient_t::get(patient_t::insert_row("John", 165, false));

    dorian.patients().insert(jane);
    house.patients().insert(jack);
    house.patients().insert(john);

    address_t seattle = address_t::get(address_t::insert_row("Seattle", "4th Ave"));
    address_t bellevue = address_t::get(address_t::insert_row("Bellevue", "8th Street"));
    address_t kissimmee = address_t::get(address_t::insert_row("Kissimmee", "Hamlet Ln"));

    jane.address().connect(seattle);
    jack.address().connect(bellevue);
    john.address().connect(kissimmee);
}

void filter_lambda()
{
    auto doctors = doctor_t::list().where(
        [](const doctor_t& d) { return strcmp(d.name(), "Dr. House") == 0; });

    if (doctors.begin() == doctors.end())
    {
        gaia_log::app().warn("No doctors found!");
        return;
    }

    // Assuming there is only one result.
    doctor_t house = *doctors.begin();

    auto patients = house.patients().where(
        [](const patient_t& p) { return strcmp(p.name(), "Jack") == 0; });

    if (patients.begin() == patients.end())
    {
        gaia_log::app().warn("No patients found!");
        return;
    }

    patient_t jack = *patients.begin();

    gaia_log::app().info("{} has patient {}", house.name(), jack.name());
}

void filter_gaia_predicates_strings()
{
    auto doctors = doctor_t::list().where(doctor_expr::name == "Dr. House");

    if (doctors.begin() == doctors.end())
    {
        gaia_log::app().warn("No doctors found!");
        return;
    }

    // Assuming there is only one result.
    doctor_t house = *doctors.begin();

    auto patients = house.patients().where(patient_expr::name == "John");

    if (patients.begin() == patients.end())
    {
        gaia_log::app().warn("No patients found!");
        return;
    }

    patient_t jack = *patients.begin();

    gaia_log::app().info("{} has patient {}", house.name(), jack.name());
}

void filter_gaia_predicates_numbers()
{
    auto female_patients = patient_t::list().where(patient_expr::is_female == true);
    gaia_log::app().info("There are {} female patients", female_patients.size());

    auto higher_than_160 = patient_t::list().where(patient_expr::height >= 160);
    gaia_log::app().info("There are {} patients higher than 160", higher_than_160.size());
}

void filter_gaia_predicates_containers()
{
    // Contains with expression.
    auto jacks_doctor_container = doctor_t::list().where(
        doctor_expr::patients.contains(
            patient_expr::name == "Jack"));

    auto jacks_doctor = *jacks_doctor_container.begin();
    gaia_log::app().info("Jack's doctor is {}", jacks_doctor.name());

    // Contains with constant.
    auto jane = *(patient_t::list().where(patient_expr::name == "Jane").begin());

    auto jane_doctor_container = doctor_t::list().where(
        doctor_expr::patients.contains(jane));

    auto janes_doctor = *jane_doctor_container.begin();
    gaia_log::app().info("Jane's doctor is {}", janes_doctor.name());

    auto doctors_with_no_patients = doctor_t::list().where(doctor_expr::patients.empty());

    if (doctors_with_no_patients.begin() == doctors_with_no_patients.end())
    {
        gaia_log::app().info("All the doctors have at least one patient");
    }

    auto doctors_with_one_patient = doctor_t::list().where(
        doctor_expr::patients.count() >= 1);

    for (doctor_t& doctor : doctors_with_one_patient)
    {
        gaia_log::app().info("{} has at least one patient", doctor.name());
    }
}

int main()
{
    gaia::system::initialize();
    gaia::db::begin_transaction();

    clean_db();

    create_record_insert();
    create_record_writer();
    lookup_record_get();
    lookup_record_get_not_existent();
    access_invalid_record();
    iterate_all_patients();
    delete_single_record();
    delete_all_records();
    gaia_id_t doctor_id = create_one_to_many_relationship();
    traverse_one_to_many_relationship(doctor_id);
    delete_one_to_many_relationship_re(doctor_id);
    delete_one_to_many_relationship(doctor_id);
    gaia_id_t patient_id = create_one_to_one_relationship();
    traverse_one_to_one_relationship(patient_id);
    delete_one_to_one_relationship_re(patient_id);
    delete_one_to_one_relationship(patient_id);
    clean_db();
    create_filter_data();
    filter_lambda();
    filter_gaia_predicates_strings();
    filter_gaia_predicates_numbers();
    filter_gaia_predicates_containers();

    gaia::db::commit_transaction();
    gaia::system::shutdown();
}
