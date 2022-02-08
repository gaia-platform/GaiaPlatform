////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <gaia/db/db.hpp>
#include <gaia/exceptions.hpp>
#include <gaia/logger.hpp>
#include <gaia/system.hpp>

#include "gaia_hospital.h"

using namespace gaia::hospital;

using gaia::common::gaia_id_t;
using gaia::direct_access::auto_transaction_t;

#define PRINT_METHOD_NAME() gaia_log::app().info("---- {}() ----", __FUNCTION__)

///
/// Direct Access API examples.
///
/// A Direct Access Class (DAC) is a C++ class representing a Gaia database table.
/// DACs are generated by the gaiac and are used to perform CRUD operations
/// on the Gaia database.
///

/**
 * Creates a record using the insert() method. insert() returns an
 * ID of type gaia_id_t that is used to look up the record.
 */
void create_record_insert()
{
    PRINT_METHOD_NAME();

    // The insert_row() method has one argument for each column
    // in the doctor table.
    gaia_id_t id = doctor_t::insert_row("Dr. House");

    gaia_log::app().info("Created doctor with ID: {}", id);
}

/**
 *  Creates a record using the writer class (patient_writer in this case).
 *  A writer class does not require that values are set for all of the
 *  fields in the class. Fields for which a value is not specified
 *  are assigned a default value. The writer returns a gaia_id_t.
 */
void create_record_writer()
{
    PRINT_METHOD_NAME();

    patient_writer patient_w;
    patient_w.name = "Emma";
    patient_w.is_active = true;
    gaia_id_t id = patient_w.insert_row();

    gaia_log::app().info("Created patient with ID: {}", id);
}

/**
 * Lookup an patient_t record using the static get(gaia_id_t) method.
 */
void lookup_record_get()
{
    PRINT_METHOD_NAME();

    gaia_id_t id = patient_t::insert_row("John", 175, false, {});

    patient_t john = patient_t::get(id);
    gaia_log::app().info("Patient name: {}", john.name());
}

/**
 * Gaia supports array type fields.
 */
void array_type_fields()
{
    PRINT_METHOD_NAME();

    patient_t john = patient_t::get(
        patient_t::insert_row("John", 175, false, {1.0, 0.9, 3.1, 0.8}));

    gaia_log::app().info("{}'s analysis results are:", john.name());

    auto analysis_results = john.analysis_results();

    // To iterate the array you can use a normal for loop.
    for (size_t i = 0; i < analysis_results.size(); i++)
    {
        gaia_log::app().info(" - {}", analysis_results[i]);
    }
}

/**
 * Records can be modified using a writer object.
 */
void update_record()
{
    PRINT_METHOD_NAME();

    gaia_id_t id = patient_t::insert_row("John", 175, false, {});

    patient_t john = patient_t::get(id);
    gaia_log::app().info("Patient name is: {}", john.name());

    // Obtain the writer object for an existing record.
    patient_writer john_w = john.writer();
    john_w.name = "Jane";
    john_w.height = 178;
    john_w.analysis_results = {1.0, 1.2, 0.3};
    john_w.is_active = true;
    john_w.update_row();

    gaia_log::app().info("Patient name now is: {}", john.name());
}

/**
 * Optional values can represent absence of value.
 */
void optional_values()
{
    PRINT_METHOD_NAME();

    // You can pass nullopt instead of height to denote the absence of value.
    gaia_id_t id = patient_t::insert_row("John", gaia::common::nullopt, false, {});
    patient_t john = patient_t::get(id);

    // john.height() return a gaia::common::optional_t<uint8> which behaves
    // similarly to a C++17 std::optional.
    if (john.height().has_value())
    {
        throw std::runtime_error("The value for john.height() should be missing.");
    }
    else
    {
        gaia_log::app().info("No height provided for patient {}", john.name());
    }

    // You can update the missing value with a value.
    patient_writer john_w = john.writer();
    john_w.height = 178;
    john_w.update_row();

    if (john.height().has_value())
    {
        gaia_log::app().info("{}'s height is {}", john.name(), john.height().value());
    }
    else
    {
        throw std::runtime_error("The height column is supposed to have a value.");
    }
}

/**
 * DACs override the bool() operator to return true when the
 * instance is in a valid state and false otherwise.
 */
void lookup_invalid_record()
{
    PRINT_METHOD_NAME();

    try
    {
        // A failure will happen for every id that does not map to an existing object.
        patient_t::get(gaia::common::c_invalid_gaia_id);
        throw std::runtime_error("patient_t::get(gaia::common::c_invalid_gaia_id) should have failed with an exception.");
    }
    catch (const gaia::db::invalid_object_id& e)
    {
        gaia_log::app().info("Getting object with invalid id failed as expected: {}", e.what());
    }
}

/**
 * When a DAC instance references an invalid database record
 * Gaia throws an invalid_object_id exception.
 */
void access_invalid_record()
{
    PRINT_METHOD_NAME();

    patient_t john;

    try
    {
        gaia_log::app().info("Patient name: {}", john.name());
        throw std::runtime_error("Accessing an invalid object is expected to fail.");
    }
    catch (gaia::direct_access::invalid_object_state& e)
    {
        gaia_log::app().info("As expected, attempting to access an invalid object raised the following exception: '{}'.", e.what());
    }
}

/**
 * DACs override the == operator which can be used to test instances equality.
 * Instances are determined to be equal when their gaia_id matches.
 */
void compare_records()
{
    PRINT_METHOD_NAME();

    gaia_id_t id = doctor_t::insert_row("Dr. Reid");

    doctor_t dr_house_1 = doctor_t::get(id);
    doctor_t dr_house_2 = doctor_t::get(id);

    if (dr_house_1 == dr_house_2)
    {
        gaia_log::app().info("The instances refer to the same object.");
    }
    else
    {
        throw std::runtime_error("The records are expected to be equal.");
    }

    doctor_t dr_dorian = doctor_t::get(doctor_t::insert_row("Dr. Dorian"));

    if (dr_dorian == dr_house_2)
    {
        throw std::runtime_error("The records are not expected to be equal.");
    }
    else
    {
        gaia_log::app().info("The instances do not refer to the same object.");
    }
}

/**
 * Each DAC class exposes a static list() method that allows you to iterate
 * through all of the records in a table.
 */
void list_all_patients()
{
    PRINT_METHOD_NAME();

    for (auto& patient : patient_t::list())
    {
        // patient.height().value_or(0) is expected to return 0 since
        // the height hasn't been assigned.
        gaia_log::app().info(
            "Patient name:{}, height:{} is_active:{}",
            patient.name(),
            patient.height().value_or(0),
            patient.is_active());
    }
}

/**
 * You can delete records by calling the delete_row() method.
 */
void delete_single_record()
{
    PRINT_METHOD_NAME();

    doctor_t dr_house = doctor_t::get(doctor_t::insert_row("Dr. House"));

    dr_house.delete_row();

    if (dr_house)
    {
        throw std::runtime_error("The doctor is expected to be invalid after deletion.");
    }
    else
    {
        gaia_log::app().info("The record has been deleted");
    }
}

/**
 * You can delete records by calling the delete_row(gaia_id_t) static method.
 * This can be useful if you have a gaia_id_t and you don't want to
 * instantiate an object to call the delete_row() instance method.
 */
void delete_single_record_static()
{
    PRINT_METHOD_NAME();

    gaia_id_t dr_house_id = doctor_t::insert_row("Dr. House");

    doctor_t::delete_row(dr_house_id);

    try
    {
        doctor_t::get(dr_house_id);
        throw std::runtime_error("The doctor is expected to be invalid after deletion.");
    }
    catch (const gaia::db::invalid_object_id& e)
    {
        gaia_log::app().info("The record has been deleted");
    }
}

/**
 * While you might attempt to list all of the records with list()
 * and call delete() on each of them, this approach will not work.
 * The container uses the current record to find the next record.
 * Deleting the current record stops the iteration because the
 * next record cannot be found.
 */
void delete_all_records()
{
    PRINT_METHOD_NAME();

    doctor_t::insert_row("Dr. House");
    doctor_t::insert_row("Dr. Dorian");
    doctor_t::insert_row("Dr. Reid");

    for (auto doctor_it = doctor_t::list().begin();
         doctor_it != doctor_t::list().end();)
    {
        auto next_doctor_it = doctor_it++;
        next_doctor_it->delete_row();
    }

    if (doctor_t::list().size() > 0)
    {
        throw std::runtime_error("The doctor table is expected to be empty.");
    }

    gaia_log::app().info("Doctors count: {}", doctor_t::list().size());
}

/**
 * Creates a one-to-many relationship between a doctor and several patients.
 *
 * @return A gaia_id used by the subsequent methods in this source file.
 */
gaia_id_t create_one_to_many_relationship()
{
    doctor_t dr_house = doctor_t::get(doctor_t::insert_row("Dr. House"));

    patient_t jane = patient_t::get(patient_t::insert_row("Jane", 183, true, {}));
    patient_t jack = patient_t::get(patient_t::insert_row("Jack", 176, false, {}));
    gaia_id_t john_id = patient_t::insert_row("John", 175, false, {});

    // Type safe insert (accepts instances of patient_t).
    dr_house.patients().insert(jane);
    dr_house.patients().insert(jack);

    // Type unsafe insert (accept gaia_id_t).
    dr_house.patients().insert(john_id);

    return dr_house.gaia_id();
}

/**
 * Iterates over all the patients of the specified doctor.
 * For each patient, traverse the backlink to the doctor.
 */
void traverse_one_to_many_relationship()
{
    PRINT_METHOD_NAME();

    gaia_id_t doctor_id = create_one_to_many_relationship();
    doctor_t doctor = doctor_t::get(doctor_id);

    // doctor.patients() returns a container of patient_t.
    // The container is compatible with CPP STL containers.
    for (auto& patient : doctor.patients())
    {
        gaia_log::app().info("Patient name: {}", patient.name());

        // Traverse the backlink from the patient to the doctor.
        gaia_log::app().info("Patient's doctor: {}", patient.doctor().name());
    }
}

/**
 * With 1:n relationships, attempting to delete a database object that
 * is on the "1" side and has at least one connected object on the "n"
 * side violates referential integrity, causing Gaia system to throw an
 * object_still_referenced exception. You can delete the "n" side first;
 * see next example.
 */
void delete_one_to_many_relationship_re()
{
    PRINT_METHOD_NAME();

    gaia_id_t doctor_id = create_one_to_many_relationship();
    doctor_t doctor = doctor_t::get(doctor_id);

    try
    {
        // You can't delete doctor because it's on the 1 side of the 1:n relationship.
        doctor.delete_row();
        throw std::runtime_error("It should not be possible to delete the doctor because it has a patient connected.");
    }
    catch (const gaia::db::object_still_referenced& e)
    {
        gaia_log::app().info("As expected, deleting the doctor record raised the following exception '{}'.", e.what());
    }
}

/**
 * There are three ways to delete an object referenced by another object in a 1:n relationship:
 * 1. Delete the objects on the "n" side first.
 * 2. Disconnect the objects on the "n" side first, and then delete the object on the "1" side.
 * 3. Delete the object on the "1" side by specifying the force=true flag in the delete_row() method.
 */
void delete_one_to_many_relationship()
{
    PRINT_METHOD_NAME();

    gaia_id_t doctor_id = create_one_to_many_relationship();
    doctor_t doctor = doctor_t::get(doctor_id);

    // Pick one of the doctor's patients.
    patient_t patient = *(doctor.patients().begin());

    // 1. You can delete the patient because it's on the "n" side of the relationship.
    patient.delete_row();

    // 2. You can unlink all the remaining patients.
    doctor.patients().clear();

    // You can now delete the doctor.
    doctor.delete_row();

    // 3. If you want to delete the doctor first,
    // you can pass the force=true flag to delete_row().
    doctor_id = create_one_to_many_relationship();
    doctor = doctor_t::get(doctor_id);
    doctor.delete_row(true);
}

/**
 * Gaia reference containers expose the erase() method which behaves like
 * the STL erase method. It removes the element at the specified position
 * and returns an iterator to the following value.
 */
void delete_one_to_many_relationship_erase()
{
    PRINT_METHOD_NAME();

    gaia_id_t doctor_id = create_one_to_many_relationship();

    doctor_t doctor = doctor_t::get(doctor_id);
    auto doctor_patients = doctor.patients();

    for (auto patient_it = doctor_patients.begin(); patient_it != doctor_patients.end();)
    {
        patient_t patient = *patient_it;
        patient_it = doctor_patients.erase(patient_it);
        patient.delete_row();
    }

    doctor.delete_row();
}

/**
 * Creates a one-to-one relationship between a patient and an address
 * by setting the patient's address.
 *
 * @return A gaia_id used by the following examples.
 */
gaia_id_t create_one_to_one_relationship()
{
    PRINT_METHOD_NAME();

    patient_t jane = patient_t::get(patient_t::insert_row("Jane", 183, true, {}));
    address_t amsterdam = address_t::get(address_t::insert_row("Tuinstraat", "Amsterdam"));

    // The address() method returns an address_ref_t which is a subclass of
    // address_t that exposes the connect()/disconnect() methods to set/unset
    // the reference.
    jane.address().connect(amsterdam);

    return jane.gaia_id();
}

/**
 * Access the address from the patient, and access the patient from the address
 * using the backlink.
 */
void traverse_one_to_one_relationship()
{
    PRINT_METHOD_NAME();

    gaia_id_t patient_id = create_one_to_one_relationship();
    patient_t patient = patient_t::get(patient_id);
    // Actually returns address_ref_t which being a subclass of
    // address_t can be assigned to address_t.
    address_t address = patient.address(); // NOLINT(cppcoreguidelines-slicing)

    gaia_log::app().info("City {}", address.city());

    // Note: address.patient() returns a patient_t and not a patient_ref_t.
    // This means you can set/unset the reference only from the patient side:
    //
    // You can do: patient.address().connect(...)
    // You can't do: address.patient().connect(...)
    //
    // This depends on the order in which items are defined in the DDL.
    // patient appears first hence it exposes the ability to set/unset
    // the reference to address.
    gaia_log::app().info("Patient {}", address.patient().name());
}

/**
 * Likewise in 1:n relationships, if you try to delete a database object that
 * is referenced by another object (in a 1:1 relationship) the Gaia system
 * will throw an object_still_referenced exception. In a 1:1 relationship you can delete
 * the object on the table that is defined later in the DDL (address in this case)
 */
void delete_one_to_one_relationship_re()
{
    PRINT_METHOD_NAME();

    gaia_id_t patient_id = create_one_to_one_relationship();
    patient_t patient = patient_t::get(patient_id);

    try
    {
        // You cannot delete patient because it's defined before address in the DDL.
        patient.delete_row();
        throw std::runtime_error("The patient deletion should fail!");
    }
    catch (const gaia::db::object_still_referenced& e)
    {
        gaia_log::app().info("As expected, deleting the patient record raised the following exception '{}'.", e.what());
    }
}

/**
 * There are three ways to delete an object referenced by another object in a 1:n relationship:
 * 1. Disconnect the objects first, and then delete it.
 * 2. Delete the object defined in the table that appear later in the DDL.
 * 3. Delete the object specifying force=true flag in the delete_row() method.
 */
void delete_one_to_one_relationship()
{
    PRINT_METHOD_NAME();

    // 1. You can disconnect the objects first and then delete them.
    gaia_id_t patient_id = create_one_to_one_relationship();
    patient_t patient = patient_t::get(patient_id);
    address_ref_t address = patient.address(); // NOLINT(cppcoreguidelines-slicing)
    patient.address().disconnect();
    patient.delete_row();
    address.delete_row();

    // 2. You can delete the address first.
    patient_id = create_one_to_one_relationship();
    patient = patient_t::get(patient_id);
    address = patient.address();
    address.delete_row();
    patient.delete_row();

    // 3. Pass the force=true flag to delete_row()
    // to delete the patient first.
    patient_id = create_one_to_one_relationship();
    patient = patient_t::get(patient_id);
    address = patient.address();
    patient.delete_row(true);
    address.delete_row();
}

/**
 * Helper method to create data used in filter examples.
 */
void create_filter_data()
{
    doctor_t dr_dorian = doctor_t::get(doctor_t::insert_row("Dr. Dorian"));
    doctor_t dr_house = doctor_t::get(doctor_t::insert_row("Dr. House"));

    patient_t jane = patient_t::get(patient_t::insert_row("Jane", 183, true, {}));
    patient_t jack = patient_t::get(patient_t::insert_row("Jack", 176, false, {}));
    patient_t john = patient_t::get(patient_t::insert_row("John", 165, false, {}));

    dr_dorian.patients().insert(jane);
    dr_house.patients().insert(jack);
    dr_house.patients().insert(john);

    address_t seattle = address_t::get(address_t::insert_row("4th Ave", "Seattle"));
    address_t bellevue = address_t::get(address_t::insert_row("8th Street", "Bellevue"));
    address_t amsterdam = address_t::get(address_t::insert_row("Tuinstraat", "Amsterdam"));

    jane.address().connect(seattle);
    jack.address().connect(bellevue);
    john.address().connect(amsterdam);
}

/**
 * The Direct Access API allows filtering of data using predicates expressed as:
 *
 *   std::function<bool (const T_class&)>
 *
 * where T_class is the type of the container the filter is applied to.
 */
void filter_lambda()
{
    PRINT_METHOD_NAME();

    // Find all the doctors whose name is "Dr. House".
    // using a lambda to express a predicate.
    // The result is a gaia container.
    auto doctors = doctor_t::list().where(
        [](const doctor_t& doctor) { return strcmp(doctor.name(), "Dr. House") == 0; });

    if (doctors.begin() == doctors.end())
    {
        throw std::runtime_error("No doctors found!");
    }

    // Takes the first item in the container (there should be only one match).
    doctor_t dr_house = *doctors.begin();

    auto patients = dr_house.patients().where(
        [](const patient_t& patient) { return strcmp(patient.name(), "Jack") == 0; });

    if (patients.begin() == patients.end())
    {
        throw std::runtime_error("No patients found!");
    }

    patient_t jack = *patients.begin();

    gaia_log::app().info("{} has patient {}", dr_house.name(), jack.name());
}

/**
 * C++ lambda syntax can be quite verbose, for this reason DACs expose
 * an API to create predicates for the all the class fields.
 *
 * This API is under the T_class_expr namespace (eg. doctor_expr for
 * doctor_t class).
 *
 * The API provides the == and != operators for string (const char*
 * and std::string).
 */
void filter_gaia_predicates_strings()
{
    PRINT_METHOD_NAME();

    // The doctor_expr namespace is generated along with the doctor_t class.
    auto doctors = doctor_t::list().where(doctor_expr::name == "Dr. House");

    if (doctors.begin() == doctors.end())
    {
        throw std::runtime_error("No doctors found!");
    }

    // Assuming there is only one result.
    doctor_t dr_house = *doctors.begin();

    auto patients = dr_house.patients().where(patient_expr::name != "John");

    for (patient_t& patient : patients)
    {
        gaia_log::app().info("{} has patient {}", dr_house.name(), patient.name());
    }
}

/**
 * For numerical values you can use: >, >=, ==, !=, <= and <.
 */
void filter_gaia_predicates_numbers()
{
    PRINT_METHOD_NAME();

    auto active_patients = patient_t::list().where(patient_expr::is_active == true);
    gaia_log::app().info("There are {} active patients", active_patients.size());

    auto higher_than_160 = patient_t::list().where(patient_expr::height >= 160);
    gaia_log::app().info("There are {} patients higher than 160", higher_than_160.size());
}

/**
 * For containers you can use contains(), empty(), count().
 */
void filter_gaia_predicates_containers()
{
    PRINT_METHOD_NAME();

    // Contains with expression.
    auto jacks_doctor_container = doctor_t::list().where(
        doctor_expr::patients.contains(
            patient_expr::name == "Jack"));

    auto jacks_doctor = *jacks_doctor_container.begin();
    gaia_log::app().info("Jack's doctor is {}", jacks_doctor.name());

    // Contains with constant.
    auto jane = *(patient_t::list().where(patient_expr::name == "Jane").begin());

    auto janes_doctor_container = doctor_t::list().where(
        doctor_expr::patients.contains(jane));

    auto janes_doctor = *janes_doctor_container.begin();
    gaia_log::app().info("Jane's doctor is {}", janes_doctor.name());

    auto doctors_with_no_patients = doctor_t::list().where(doctor_expr::patients.empty());

    if (doctors_with_no_patients.begin() == doctors_with_no_patients.end())
    {
        gaia_log::app().info("All the doctors have at least one patient");
    }

    auto doctors_with_patients = doctor_t::list().where(
        doctor_expr::patients.count() >= 1);

    for (doctor_t& doctor : doctors_with_patients)
    {
        gaia_log::app().info("{} has at least one patient", doctor.name());
    }
}

/**
 * DAC objects can be used across different transactions.
 *
 * This example uses auto_transaction_t to manage transactions.
 * See gaia/direct_access/auto_transaction.hpp for more information
 * about auto_transaction_t usage.
 */
void use_dac_object_across_transactions()
{
    PRINT_METHOD_NAME();

    // First transaction.
    auto_transaction_t txn{auto_transaction_t::no_auto_restart};
    doctor_t dr_house = doctor_t::get(doctor_t::insert_row("Dr. House"));
    txn.commit();

    try
    {
        // Outside a transaction.
        dr_house.name();
        throw std::runtime_error("It should not be possible to interact with the database outside of a transaction.");
    }
    catch (const gaia::db::no_open_transaction& e)
    {
        gaia_log::app().info("As expected, you cannot access a record outside of a transaction: '{}'", e.what());
    }

    // Second transaction.
    txn.begin();
    gaia_log::app().info("{} has survived across transactions", dr_house.name());
    txn.commit();
}

/**
 * Clean the database.
 */
void clean_db()
{
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
        next_patient_it->address().disconnect();
        next_patient_it->delete_row();
    }

    for (auto address_it = address_t::list().begin();
         address_it != address_t::list().end();)
    {
        auto next_address_it = address_it++;
        next_address_it->delete_row();
    }
}

int main()
{
    gaia::system::initialize();

    // The no_auto_restart argument prevents beginning a new transaction
    // when the current one is committed.
    auto_transaction_t txn{auto_transaction_t::no_auto_restart};

    clean_db();

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
    delete_one_to_one_relationship_re();
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

    gaia::system::shutdown();
}
