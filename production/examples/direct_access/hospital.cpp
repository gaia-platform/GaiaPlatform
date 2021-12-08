/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

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
    gaia_id_t id = doctor_t::insert_row("Dr. House", "house@md.com");

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

    gaia_id_t id = patient_t::insert_row("John", 175, false, nullptr, {});

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
        patient_t::insert_row("John", 175, false, nullptr, {1.0, 0.9, 3.1, 0.8}));

    gaia_log::app().info("{}'s analysis results are:", john.name());

    auto analysis_results = john.analysis_results();

    // To iterate the array you can use a normal for loop.
    for (int i = 0; i < analysis_results.size(); i++)
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

    gaia_id_t id = patient_t::insert_row("John", 175, false, nullptr, {});

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
 * DACs override the bool() operator to return true when the
 * instance is in a valid state and false otherwise.
 */
void lookup_invalid_record()
{
    PRINT_METHOD_NAME();

    try
    {
        patient_t john = patient_t::get(gaia::common::c_invalid_gaia_id);
        throw std::runtime_error("patient_t::get(gaia::common::c_invalid_gaia_id) should have failed with an exception.");
    }
    catch (const gaia::db::invalid_object_id& e)
    {
        gaia_log::app().info("Cannot find patient with id: {}", gaia::common::c_invalid_gaia_id);
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

    gaia_id_t id = doctor_t::insert_row("Dr. Reid", "reid@md.com");

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

    doctor_t dr_dorian = doctor_t::get(doctor_t::insert_row("Dr. Dorian", "dorian@md.com"));

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
        gaia_log::app().info(
            "Patient name:{}, height:{} is_active:{}", patient.name(), patient.height(), patient.is_active());
    }
}

/**
 * You can delete records by calling the delete_row() method.
 */
void delete_single_record()
{
    PRINT_METHOD_NAME();

    doctor_t dr_house = doctor_t::get(doctor_t::insert_row("Dr. House", "house@md.com"));

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

    gaia_id_t dr_house_id = doctor_t::insert_row("Dr. House", "house@md.com");

    doctor_t::delete_row(dr_house_id);

    try
    {
        doctor_t dr_house = doctor_t::get(dr_house_id);
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

    doctor_t::insert_row("Dr. House", "house@md.com");
    doctor_t::insert_row("Dr. Dorian", "dorian@md.com");
    doctor_t::insert_row("Dr. Reid", "reid@md.com");

    for (auto doctor_it = doctor_t::list().begin();
         doctor_it != doctor_t::list().end();)
    {
        auto next_doctor_it = doctor_it++;
        next_doctor_it->delete_row();
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
    doctor_t dr_house = doctor_t::get(doctor_t::insert_row("Dr. House", "house@md.com"));

    patient_t jane = patient_t::get(patient_t::insert_row("Jane", 183, true, nullptr, {}));
    patient_t jack = patient_t::get(patient_t::insert_row("Jack", 176, false, nullptr, {}));
    gaia_id_t john_id = patient_t::insert_row("John", 175, false, nullptr, {});

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
 *
 * @param doctor_id Id of previously created doctor with patients.
 */
void traverse_one_to_many_relationship(gaia_id_t doctor_id)
{
    PRINT_METHOD_NAME();

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
 * Attempting to delete a database object that is referenced by another
 * object violates referential integrity and causes Gaia to throw an
 * object_still_referenced exception.
 *
 * @param doctor_id Id of previously created doctor with patients.
 */
void delete_one_to_many_relationship_re(gaia_id_t doctor_id)
{
    PRINT_METHOD_NAME();

    doctor_t doctor = doctor_t::get(doctor_id);

    // Pick one of the doctor's patients.
    patient_t patient = *(doctor.patients().begin());

    try
    {
        doctor.delete_row();
    }
    catch (const gaia::db::object_still_referenced& e)
    {
        gaia_log::app().info("As expected, deleting the doctor record raised the following exception '{}'.", e.what());
    }

    try
    {
        patient.delete_row();
    }
    catch (const gaia::db::object_still_referenced& e)
    {
        gaia_log::app().info("As expected, deleting the patient record raised the following exception '{}'.", e.what());
    }
}

/**
 * The correct to delete an object that is referenced by other objects
 * is to disconnect it first, and then delete it.
 *
 * @param doctor_id Id of previously created doctor with patients.
 */
void delete_one_to_many_relationship(gaia_id_t doctor_id)
{
    PRINT_METHOD_NAME();

    doctor_t doctor = doctor_t::get(doctor_id);

    // Pick one of the doctor's patients.
    patient_t patient = *(doctor.patients().begin());

    // You can unlink a single element (there are still 2 connected).
    doctor.patients().remove(patient);

    // You can now delete the patient.
    patient.delete_row();

    // Unlink all the remaining patients.
    doctor.patients().clear();

    // You can now delete the doctor.
    doctor.delete_row();
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

    patient_t jane = patient_t::get(patient_t::insert_row("Jane", 183, true, nullptr, {}));
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
 *
 * @param patient_id Id of previously created patient with an address.
 */
void traverse_one_to_one_relationship(gaia_id_t patient_id)
{
    PRINT_METHOD_NAME();

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
 * is referenced by another object (in a 1:1 relationship) Gaia throw an
 * object_still_referenced exception.
 *
 * @param patient_id Id of previously created patient with an address.
 */
void delete_one_to_one_relationship_re(gaia_id_t patient_id)
{
    PRINT_METHOD_NAME();

    patient_t patient = patient_t::get(patient_id);
    address_ref_t address = patient.address();

    try
    {
        patient.delete_row();
    }
    catch (const gaia::db::object_still_referenced& e)
    {
        gaia_log::app().info("As expected, deleting the patient record raised the following exception '{}'.", e.what());
    }

    try
    {
        address.delete_row();
    }
    catch (const gaia::db::object_still_referenced& e)
    {
        gaia_log::app().info("As expected, deleting the address record raised the following exception '{}'.", e.what());
    }
}

/**
 * The correct way to delete objects in 1:1 relationships is to first disconnect them.
 *
 * @param patient_id Id of previously created patient with an address.
 */
void delete_one_to_one_relationship(gaia_id_t patient_id)
{
    PRINT_METHOD_NAME();

    patient_t patient = patient_t::get(patient_id);
    address_ref_t address = patient.address(); // NOLINT(cppcoreguidelines-slicing)

    patient.address().disconnect();

    patient.delete_row();
    address.delete_row();
}

/**
 * Shows the usage of value linked relationships. Traditional Gaia relationships
 * link records together using the Gaia ID, a system generate value the user have no
 * control over. Value linked relationships allow linking records using user specified
 * values.
 *
 * In this example we are going to link patients to a doctor using the doctor email.
 * As you can see there is no explict call to link the records (insert()/connect()),
 * the records are automatically linked when the a patient is assigned an existing
 * doctor email.
 */
void one_to_many_value_linked_relationship()
{
    PRINT_METHOD_NAME();

    // --- Create the relationship ---
    gaia_id_t dr_house_id = doctor_t::insert_row("Dr. House", "house@md.com");

    // By setting the patient_email field to "house@md.com" the patient
    // will be automatically linked to the doctor with that email.
    gaia_id_t jane_id = patient_t::insert_row("Jane", 183, true, "house@md.com", {});
    gaia_id_t jack_id = patient_t::insert_row("Jack", 176, false, "house@md.com", {});

    // John is not immediately connected with the doctor...
    patient_t john = patient_t::get(
        patient_t::insert_row("John", 175, false, "", {}));

    // The connection with the doctor is created with this update.
    patient_writer john_w = john.writer();
    john_w.doctor_email = "house@md.com";
    john_w.update_row();

    // If you set an email that does not correspond to any doctor,
    // no connection is created. You can delete the record right away.
    gaia_id_t jasmine_id = patient_t::insert_row("Jasmine", 154, false, "i_dont_exist@md.com", {});
    patient_t::delete_row(jasmine_id);

    // --- Traverse the relationship ---

    doctor_t dr_house = doctor_t::get(dr_house_id);
    for (patient_t& patient : dr_house.patients())
    {
        gaia_log::app().info("Patient:{} has doctor:{}", patient.name(), patient.doctor().name());
    }

    // --- Disconnect the relationship ---

    patient_t jane = patient_t::get(jane_id);
    patient_t jack = patient_t::get(jack_id);

    patient_writer jane_w = jane.writer();
    patient_writer jack_w = jack.writer();
    john_w = john.writer();

    // By setting to "" the doctor_email field the relationship
    // is automatically disconnected.

    jane_w.doctor_email = "";
    jane_w.update_row();
    john_w.doctor_email = "";
    john_w.update_row();
    jack_w.doctor_email = "";
    jack_w.update_row();

    if (dr_house.patients().size() > 0)
    {
        throw std::runtime_error("The doctor is supposed to have no patients!");
    }

    gaia_log::app().info("{} patients count {}", dr_house.name(), dr_house.patients().size());

    // Now you can delete all the objects.
    jane.delete_row();
    john.delete_row();
    jack.delete_row();
    dr_house.delete_row();
}

/**
 * Helper method to create data used in filter examples.
 */
void create_filter_data()
{
    doctor_t dr_dorian = doctor_t::get(doctor_t::insert_row("Dr. Dorian", "dorian@md.com"));
    doctor_t dr_house = doctor_t::get(doctor_t::insert_row("Dr. House", "house@md.com"));

    patient_t jane = patient_t::get(patient_t::insert_row("Jane", 183, true, nullptr, {}));
    patient_t jack = patient_t::get(patient_t::insert_row("Jack", 176, false, nullptr, {}));
    patient_t john = patient_t::get(patient_t::insert_row("John", 165, false, nullptr, {}));

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
        [](const doctor_t& doctor) { return strcmp(doctor.email(), "house@md.com") == 0; });

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
    auto_transaction_t txn{auto_transaction_t::no_auto_begin};
    doctor_t dr_house = doctor_t::get(doctor_t::insert_row("Dr. House", "house@md.com"));
    txn.commit();

    try
    {
        // Outside a transaction.
        dr_house.name();
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

    // The no_auto_begin argument prevents beginning a new transaction
    // when the current one is committed.
    auto_transaction_t txn{auto_transaction_t::no_auto_begin};

    clean_db();

    create_record_insert();
    create_record_writer();
    lookup_record_get();
    array_type_fields();
    update_record();
    lookup_invalid_record();
    access_invalid_record();
    compare_records();
    list_all_patients();
    clean_db();
    delete_single_record();
    delete_single_record_static();
    delete_all_records();
    gaia_id_t doctor_id = create_one_to_many_relationship();
    traverse_one_to_many_relationship(doctor_id);
    delete_one_to_many_relationship_re(doctor_id);
    delete_one_to_many_relationship(doctor_id);
    delete_one_to_many_relationship_erase();
    gaia_id_t patient_id = create_one_to_one_relationship();
    traverse_one_to_one_relationship(patient_id);
    delete_one_to_one_relationship_re(patient_id);
    delete_one_to_one_relationship(patient_id);
    clean_db();
    one_to_many_value_linked_relationship();
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
