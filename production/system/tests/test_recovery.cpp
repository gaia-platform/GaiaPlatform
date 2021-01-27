/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include <unistd.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "gaia/db/catalog.hpp"
#include "gaia/db/db.hpp"
#include "db_test_util.hpp"
#include "gaia_addr_book.h"
#include "schema_loader.hpp"
#include "type_id_mapping.hpp"

#include "gaia_internal/catalog/ddl_executor.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/logger_internal.hpp"
#include "gaia_internal/db/db_test_helpers.hpp"

using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::addr_book;
using namespace gaia::db::test;

using std::string;

// TODO (Mihir) - Run with ctest?
// Sample usage:
// test_recovery "/home/ubuntu/GaiaPlatform/production/build/db/core"

class recovery_test : public ::testing::Test
{
public:
    // Empty server path, enable persistence.
    static inline db_server_t s_server{nullptr, false};

    static void set_server_path(const char* server_path)
    {
        s_server.set_path(server_path);
    }

    // Write 16 records in a single transaction.
    static constexpr size_t c_load_batch_size = 16;
    // Size of a single record.
    static constexpr size_t c_employee_record_size_bytes = 648;
    // Size of a string field in a record.
    static constexpr size_t c_field_size_bytes = 128;

    // Don't cache direct access objects as they will
    // point to garbage values post crash recovery.
    struct employee_copy_t
    {
        string name_first;
        string name_last;
        string ssn;
        int64_t hire_date;
        string email;
        string web;
    };

    // Helpers used by test methods.
    static void validate_data();

    static gaia_id_t get_random_map_key(map<gaia_id_t, employee_copy_t> m);

    static string generate_string(size_t length_in_bytes);

    static void modify_data();

    static employee_t generate_employee_record();

    static void load_data(uint64_t total_size_bytes, bool kill_server_during_load);

    static int get_count();

    static void delete_all(int initial_record_count);

    static void load_modify_recover_test(
        uint64_t load_size_bytes, int crash_validate_loop_count, bool kill_during_workload);

    static void ensure_uncommitted_value_absent_on_restart_and_commit_new_txn_test();

    static void ensure_uncommitted_value_absent_on_restart_and_rollback_new_txn();

protected:
    void SetUp() override
    {
        s_server.stop();
        s_server.start();
        begin_session();
        type_id_mapping_t::instance().clear();
        gaia::catalog::ddl_executor_t::get().reset();
        schema_loader_t::instance().load_schema("addr_book.ddl");
        end_session();
        s_server.stop();
    }

    void TearDown() override
    {
        s_server.stop();
    }

private:
    // Map of employees for which the server has returned a successful commit.
    // We maintain this map in memory & will use it to validate recovered shared memory post crash.
    static inline std::map<gaia_id_t, employee_copy_t> s_employee_map{};
};

void recovery_test::validate_data()
{
    size_t count = 0;
    begin_transaction();
    for (auto employee = employee_t::get_first(); employee; employee = employee.get_next())
    {
        auto it = s_employee_map.find(employee.gaia_id());

        if (it == s_employee_map.end())
        {
            // There might be other tests that create employees.
            continue;
        }

        auto employee_expected = it->second;

        ASSERT_STREQ(employee_expected.email.data(), employee.email());
        ASSERT_STREQ(employee_expected.name_last.data(), employee.name_last());
        ASSERT_STREQ(employee_expected.name_first.data(), employee.name_first());
        ASSERT_STREQ(employee_expected.ssn.data(), employee.ssn());
        ASSERT_EQ(employee_expected.hire_date, employee.hire_date());
        ASSERT_STREQ(employee_expected.web.data(), employee.web());

        count++;
    }

    cout << "Total count before recovery " << s_employee_map.size() << endl;
    cout << "Total count after recovery " << count << endl;
    ASSERT_EQ(count, s_employee_map.size());
    commit_transaction();
    cout << "Validation complete." << endl;
}

gaia_id_t recovery_test::get_random_map_key(map<gaia_id_t, employee_copy_t> m)
{
    auto it = m.begin();
    std::advance(it, rand() % m.size());
    return it->first;
}

string recovery_test::generate_string(size_t length_in_bytes)
{
    auto randchar = []() -> char {
        const char charset[] = "0123456789"
                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[rand() % max_index];
    };
    string str(length_in_bytes, 0);
    generate_n(str.begin(), length_in_bytes, randchar);
    return str;
}

// Random updates & deletes.
void recovery_test::modify_data()
{
    std::set<gaia_id_t> to_delete_set;
    for (size_t i = 0; i < s_employee_map.size() / 2; i++)
    {
        begin_transaction();
        auto to_update = s_employee_map.find(get_random_map_key(s_employee_map));
        employee_t e1 = employee_t::get(to_update->first);
        auto w1 = e1.writer();
        auto name_first = generate_string(c_field_size_bytes);
        w1.name_first = name_first;
        w1.update_row();
        commit_transaction();
        to_update->second.name_first = name_first;

        auto to_delete = s_employee_map.find(get_random_map_key(s_employee_map));
        to_delete_set.insert(to_delete->first);
    }

    cout << "[Modify record] Number of records to delete: " << to_delete_set.size() << endl;

    for (gaia_id_t id : to_delete_set)
    {
        s_employee_map.erase(id);
        begin_transaction();
        auto e = employee_t::get(id);
        e.delete_row();
        commit_transaction();
    }
}

// Method will generate an employee record of size 128 * 5 + 8 (648) bytes.
employee_t recovery_test::generate_employee_record()
{
    auto w = employee_writer();
    w.name_first = generate_string(c_field_size_bytes);
    w.name_last = generate_string(c_field_size_bytes);
    w.ssn = generate_string(c_field_size_bytes);
    w.hire_date = rand();
    w.email = generate_string(c_field_size_bytes);
    w.web = generate_string(c_field_size_bytes);

    gaia_id_t id = w.insert_row();
    return employee_t::get(id);
}

void recovery_test::load_data(uint64_t total_size_bytes, bool kill_server_during_load)
{
    auto records = total_size_bytes / c_employee_record_size_bytes + 1;

    auto number_of_transactions = records / c_load_batch_size + 1;

    cout << "Loading data: Total number of records " << records << endl;
    cout << "Loading data: Total number of transactions " << number_of_transactions << endl;

    // Load data in multiple transactions.
    for (uint64_t txn_id = 1; txn_id <= number_of_transactions; txn_id++)
    {
        // Load a batch per transaction.
        std::map<gaia_id_t, employee_copy_t> temp_employee_map;
        begin_transaction();
        for (uint64_t batch_count = 1; batch_count <= c_load_batch_size; batch_count++)
        {
            // Insert row.
            auto e = generate_employee_record();
            temp_employee_map.insert(
                make_pair(e.gaia_id(), employee_copy_t{e.name_first(), e.name_last(), e.ssn(), e.hire_date(), e.email(), e.web()}));
        }
        commit_transaction();

        s_employee_map.insert(temp_employee_map.begin(), temp_employee_map.end());
        temp_employee_map.clear();
        ASSERT_EQ(temp_employee_map.size(), 0);

        // Crash during load.
        const uint8_t count_crash = 5;
        if (kill_server_during_load && txn_id % count_crash == 0)
        {
            cout << "Crash during load" << endl;
            end_session();
            s_server.start_and_retain_persistent_store();
            begin_session();
            validate_data();
        }

        const uint8_t count_tx_interval = 25;
        if (txn_id % count_tx_interval == 0)
        {
            cout << "Loading data: Executed " << txn_id << " transactions ..." << endl;
        }
    }

    cout << "Load completed for " << s_employee_map.size() << " records." << endl;
}

int recovery_test::get_count()
{
    int total_count = 0;
    begin_transaction();
    for (auto employee = employee_t::get_first(); employee; employee = employee.get_next())
    {
        total_count++;
    }
    commit_transaction();
    return total_count;
}

void recovery_test::delete_all(int initial_record_count)
{
    cout << "Deleting all records" << endl;
    begin_transaction();
    int total_count = 0;

    // Cache entries to delete.
    std::set<gaia_id_t> to_delete;
    for (auto employee = employee_t::get_first(); employee; employee = employee.get_next())
    {
        total_count++;
        to_delete.insert(employee.gaia_id());
    }
    cout << "To delete " << total_count << " records " << endl;
    commit_transaction();

    int count = 0;

    while (true)
    {
        begin_transaction();
        auto to_delete_copy = to_delete;
        for (gaia_id_t id : to_delete_copy)
        {
            auto e = employee_t::get(id);
            try
            {
                e.delete_row();
            }
            catch (const node_not_disconnected& e)
            {
                continue;
            }
            count++;
            s_employee_map.erase(id);
            to_delete.erase(id);
        }
        commit_transaction();
        cout << "Remaining count " << get_count() << endl;
        if (get_count() == 0 || get_count() == initial_record_count)
        {
            break;
        }
    }

    cout << "Deleted " << count << " records " << endl;
    validate_data();
}

void recovery_test::load_modify_recover_test(uint64_t load_size_bytes, int crash_validate_loop_count, bool kill_during_workload)
{
    int initial_record_count;
    s_server.start_and_retain_persistent_store();
    begin_session();
    initial_record_count = get_count();
    delete_all(initial_record_count);
    load_data(load_size_bytes, kill_during_workload);
    validate_data();
    end_session();

    // Restart server, modify & validate data.
    for (int i = 0; i < crash_validate_loop_count; i++)
    {
        s_server.start_and_retain_persistent_store();
        begin_session();
        cout << "Count post recovery: " << get_count() << endl;
        validate_data();
        cout << "Modify begin: " << endl;
        modify_data();
        cout << "Modify complete: " << endl;
        validate_data();
        end_session();
    }

    s_server.start_and_retain_persistent_store();
    begin_session();
    delete_all(initial_record_count);
    end_session();

    // Validate all data deleted.
    s_server.start_and_retain_persistent_store();
    begin_session();
    ASSERT_TRUE(get_count() == initial_record_count || get_count() == 0);
    end_session();
    s_server.stop();
}

void recovery_test::ensure_uncommitted_value_absent_on_restart_and_commit_new_txn_test()
{
    gaia_id_t id;
    s_server.start_and_retain_persistent_store();
    begin_session();
    begin_transaction();
    auto e1 = generate_employee_record();
    id = e1.gaia_id();
    rollback_transaction();
    end_session();

    s_server.start_and_retain_persistent_store();
    begin_session();
    begin_transaction();
    ASSERT_FALSE(employee_t::get(id));
    // Check logging + commit path functional.
    auto e2 = generate_employee_record();
    id = e2.gaia_id();
    auto name_first = e2.name_first();
    ASSERT_EQ(employee_t::get(id).gaia_id(), id);
    ASSERT_STREQ(employee_t::get(id).name_first(), name_first);
    commit_transaction();
    end_session();
}

void recovery_test::ensure_uncommitted_value_absent_on_restart_and_rollback_new_txn()
{
    gaia_id_t id;
    s_server.start_and_retain_persistent_store();
    begin_session();
    begin_transaction();
    auto e1 = generate_employee_record();
    id = e1.gaia_id();
    rollback_transaction();
    end_session();

    s_server.start_and_retain_persistent_store();
    begin_session();
    begin_transaction();
    ASSERT_FALSE(employee_t::get(id));
    // Check logging + rollback functional.
    auto e2 = generate_employee_record();
    id = e2.gaia_id();
    auto name_first = e2.name_first();
    ASSERT_EQ(employee_t::get(id).gaia_id(), id);
    ASSERT_STREQ(employee_t::get(id).name_first(), name_first);
    rollback_transaction();
    end_session();
}

// TODO (Mihir) Validate gaia_id is not recycled post crash.

TEST_F(recovery_test, reference_update_test)
{
    s_server.start_and_retain_persistent_store();
    begin_session();
    gaia_id_t address_id{c_invalid_gaia_id};
    {
        // Add an address.
        auto_transaction_t txn;
        address_writer w;
        w.street = generate_string(c_field_size_bytes);
        w.apt_suite = generate_string(c_field_size_bytes);
        w.city = generate_string(c_field_size_bytes);
        w.state = generate_string(c_field_size_bytes);
        w.postal = generate_string(c_field_size_bytes);
        w.country = generate_string(c_field_size_bytes);
        w.current = true;
        address_id = w.insert_row();
        txn.commit();
    }
    ASSERT_NE(address_id, c_invalid_gaia_id);

    std::set<gaia_id_t> phone_ids;
    {
        // Insert some phone records.
        const size_t count_rows = 10;
        auto_transaction_t txn;
        for (size_t i = 0; i < count_rows; i++)
        {
            phone_writer w;
            w.phone_number = generate_string(c_field_size_bytes);
            w.type = generate_string(c_field_size_bytes);
            w.primary = true;
            gaia_id_t phone_id = w.insert_row();
            ASSERT_NE(phone_id, c_invalid_gaia_id);
            phone_ids.insert(phone_id);
        }
        txn.commit();
    }
    {
        // Link the phone records to the address.
        auto_transaction_t txn;
        for (gaia_id_t phone_id : phone_ids)
        {
            address_t::get(address_id).phone_list().insert(phone_id);
        }
        txn.commit();
    }
    end_session();

    s_server.start_and_retain_persistent_store();
    begin_session();
    std::set<gaia_id_t> recovered_phone_ids;
    {
        auto_transaction_t txn;
        // Make sure address cannot be deleted upon recovery.
        ASSERT_THROW(address_t::get(address_id).delete_row(), node_not_disconnected);
        for (auto const& phone : address_t::get(address_id).phone_list())
        {
            recovered_phone_ids.insert(phone.gaia_id());
        }
        // Make sure the references are recovered.
        ASSERT_EQ(phone_ids, recovered_phone_ids);

        // Delete links between the phone records and the address record.
        for (gaia_id_t phone_id : recovered_phone_ids)
        {
            address_t::get(address_id).phone_list().erase(phone_id);
        }
        txn.commit();
    }
    end_session();

    s_server.start_and_retain_persistent_store();
    begin_session();
    begin_transaction();
    auto phone_list = address_t::get(address_id).phone_list();
    // Make sure the references are deleted on recovery.
    ASSERT_EQ(phone_list.begin(), phone_list.end());
    commit_transaction();
    end_session();
}

TEST_F(recovery_test, reference_create_delete_test_new)
{
    constexpr int c_num_children = 10;
    gaia_id_t parent_id;
    std::vector<gaia_id_t> children_ids{};

    s_server.start_and_retain_persistent_store();
    begin_session();
    {
        auto_transaction_t txn;

        // Create the relationship.
        relationship_builder_t::one_to_many()
            .parent(c_doctor_type)
            .child(c_patient_type)
            .create_relationship();

        // Create the parent.
        gaia_ptr parent = create_object(c_doctor_type, "Dr. House");
        parent_id = parent.id();

        // Create the children.
        for (int i = 0; i < c_num_children; i++)
        {
            gaia_ptr child = create_object(c_patient_type, "John Doe " + std::to_string(i));

            // Add half references from the parent and half from the children.
            // (semantically same operation)
            if (i < 5)
            {
                parent.add_child_reference(child.id(), c_first_patient_offset);
            }
            else
            {
                child.add_parent_reference(parent_id, c_parent_doctor_offset);
            }
        }
        txn.commit();
    }
    end_session();

    s_server.start_and_retain_persistent_store();

    begin_session();
    {
        auto_transaction_t txn;

        // Get the parent.
        gaia_ptr parent = gaia_ptr::open(parent_id);
        // Make sure address cannot be deleted upon recovery.
        ASSERT_THROW(gaia_ptr::remove(parent), node_not_disconnected);

        // Find the children.
        gaia_ptr first_child = gaia_ptr::open(parent.references()[c_first_patient_offset]);
        children_ids.push_back(first_child.id());
        gaia_ptr next_child = gaia_ptr::open(first_child.references()[c_next_patient_offset]);

        while (next_child)
        {
            children_ids.push_back(next_child.id());
            next_child = gaia_ptr::open(next_child.references()[c_next_patient_offset]);
        }

        // Ensure parent has all the children
        ASSERT_EQ(c_num_children, children_ids.size());

        // Delete the children.
        for (int i = 0; i < c_num_children; i++)
        {
            gaia_id_t child_id = children_ids[i];

            // Remove half references from the parent and half from the children.
            // (semantically same operation)
            if (i < 5)
            {
                parent.remove_child_reference(child_id, c_first_patient_offset);
            }
            else
            {
                gaia_ptr child = gaia_ptr::open(child_id);
                child.remove_parent_reference(parent_id, c_parent_doctor_offset);
            }
        }
        txn.commit();
    }
    end_session();

    s_server.start_and_retain_persistent_store();

    begin_session();
    {
        auto_transaction_t txn;

        // Get the parent.
        gaia_ptr parent = gaia_ptr::open(parent_id);

        // Ensure the parent does not have children
        ASSERT_EQ(c_invalid_gaia_id, parent.references()[c_first_patient_offset]);
        txn.commit();
    }
    end_session();
}

TEST_F(recovery_test, reference_update_test_new)
{
    gaia_id_t parent_id;
    gaia_id_t child_id;
    gaia_id_t new_parent_id;

    s_server.start_and_retain_persistent_store();
    begin_session();
    {
        auto_transaction_t txn;

        // Create the relationship.
        relationship_builder_t::one_to_many()
            .parent(c_doctor_type)
            .child(c_patient_type)
            .create_relationship();

        // Create the parent.
        gaia_ptr parent = create_object(c_doctor_type, "Dr. House");
        parent_id = parent.id();

        // Create child.
        gaia_ptr child = create_object(c_patient_type, "John Doe ");
        child_id = child.id();

        parent.add_child_reference(child_id, c_first_patient_offset);

        txn.commit();
    }
    end_session();

    s_server.start_and_retain_persistent_store();

    begin_session();
    {
        auto_transaction_t txn;

        // Create the new parent.
        gaia_ptr new_parent = create_object(c_doctor_type, "Dr. House");
        new_parent_id = new_parent.id();

        // Get the child
        gaia_ptr child = gaia_ptr::open(child_id);
        child.update_parent_reference(new_parent_id, c_parent_doctor_offset);

        txn.commit();
    }
    end_session();

    s_server.start_and_retain_persistent_store();

    begin_session();
    {
        auto_transaction_t txn;

        gaia_ptr parent = gaia_ptr::open(parent_id);
        gaia_ptr child = gaia_ptr::open(child_id);
        gaia_ptr new_parent = gaia_ptr::open(new_parent_id);

        ASSERT_EQ(c_invalid_gaia_id, parent.references()[c_first_patient_offset]);
        ASSERT_EQ(new_parent_id, child.references()[c_parent_doctor_offset]);
        ASSERT_EQ(c_invalid_gaia_id, child.references()[c_next_patient_offset]);
        ASSERT_EQ(child_id, new_parent.references()[c_first_patient_offset]);

        txn.commit();
    }
    end_session();
}

TEST_F(recovery_test, basic_correctness_test)
{
    // Basic correctness test.
    ensure_uncommitted_value_absent_on_restart_and_commit_new_txn_test();
    ensure_uncommitted_value_absent_on_restart_and_rollback_new_txn();
}

TEST_F(recovery_test, load_and_recover_test)
{
    // Load & Recover test - with data size less than write buffer size;
    // All writes will be confined to the WAL & will not make it to SST (DB binary file)
    // Sigkill server.
    const uint64_t load_size = 0.1 * 1024 * 1024;
    load_modify_recover_test(load_size, 2, true);
}

TEST_F(recovery_test, DISABLED_load_more_data_and_recover_test)
{
    const uint64_t load_size = 16 * 1024 * 1024;
    // Load (more data) & Recover test - with data size greater than write buffer size.
    // Writes will exist in both the WAL & SST files.
    // TODO - Test is switched off as it takes some time to run. Run on teamcity.
    load_modify_recover_test(load_size, 1, false);
}

int main(int argc, char** argv)
{
    gaia_log::initialize({});

    if (argc != 2)
    {
        gaia_log::db().critical("You must specify the gaia_db_server path. eg:\n\n "
                                " test_recovery \"production/build/db/core\"");
        exit(1);
    }

    ::testing::InitGoogleTest(&argc, argv);
    recovery_test::set_server_path(argv[1]);
    return RUN_ALL_TESTS();
}
