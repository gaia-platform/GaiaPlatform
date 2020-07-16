/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include "gtest/gtest.h"
#include "addr_book_gaia_generated.h"
#include "event_manager.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::rules;
using namespace gaia::common;
using namespace AddrBook;

uint32_t g_initialize_rules_called = 0;
static uint32_t rule_count = 0;
const gaia_type_t m_gaia_type = 0;
extern "C"
void initialize_rules()
{
    ++g_initialize_rules_called;
}

void rule1(const rule_context_t* context)
{
    rule_count++;
}

rule_binding_t m_rule1{"ruleset1_name", "rule1_name", rule1};

class gaia_object_test : public ::testing::Test {
public:
    static bool init_rules;
protected:
    void delete_employees() {
        Employee* e;
        begin_transaction();
        for(e = Employee::get_first();
            e;
            e = Employee::get_first())
        {
            e->delete_row();
            delete e;
        }
        commit_transaction();
    }

    void SetUp() override {
        gaia_mem_base::init(true);
        if (!init_rules) {
            init_rules = true;
            gaia::rules::initialize_rules_engine();
        } 

    }

    void TearDown() override {
        delete_employees();
        // Delete the shared memory segments.
        gaia_mem_base::reset();
    }
};

bool gaia_object_test::init_rules;

int count_rows() {
    int count = 0;
    for (auto e = Employee::get_first();
         e;
         e = e->get_next())
    {
        ++count;
    }
    return count;
}

Employee* set_field(const char* name) {
    auto e = new Employee();
    e->set_name_first(name);
    return e;
}

Employee* insert_row(const char* name) {
    auto e = set_field(name);
    EXPECT_NE(e, nullptr);
    e->insert_row();
    return e;
}

Employee* update_row(const char* name) {
    auto e = insert_row(name);
    EXPECT_NE(e, nullptr);
    e->update_row();
    return e;
}

Employee* get_field(const char* name) {
    auto e = update_row(name);
    EXPECT_STREQ(e->name_first(), name);
    return e;
}

// Test on objects created by new()
// ================================

// Create, write & read, one row
TEST_F(gaia_object_test, get_field) {
    subscribe_rule(m_gaia_type, event_type_t::row_insert, empty_fields, m_rule1);
    begin_transaction();
    get_field("Harold");
    commit_transaction();
    assert(rule_count == 1);
    unsubscribe_rules();
}

// Delete one row
TEST_F(gaia_object_test, get_field_delete) {
    begin_transaction();
    auto e = get_field("Jameson");
    e->delete_row();
    delete e;
    commit_transaction();
}

// Scan multiple rows
TEST_F(gaia_object_test, new_set_ins) {
    begin_transaction();
    get_field("Harold");
    get_field("Jameson");
    EXPECT_EQ(2, count_rows());
    commit_transaction();
}

// Read back from new, unsaved object
TEST_F(gaia_object_test, net_set_get) {
    begin_transaction();
    auto e = new Employee();
    e->set_name_last("Smith");
    EXPECT_STREQ(e->name_last(), "Smith");
    EXPECT_STREQ(e->name_last_original(), "Smith");
    // note: no row inserted
    delete e;
    commit_transaction();
}

// Attempt to read original value from new object
TEST_F(gaia_object_test, read_original_from_copy) {
    begin_transaction();
    auto e = get_field("Zachary");
    EXPECT_STREQ("Zachary", e->name_first_original());
    EXPECT_STREQ("Zachary", e->name_first());
    commit_transaction();
}

// Insert a row with no field values
TEST_F(gaia_object_test, new_ins_get) {
    begin_transaction();
    
    auto e = new Employee();
    e->insert_row();
    auto name = e->name_first();
    auto hire_date = e->hire_date();

    EXPECT_EQ(name, nullptr);
    EXPECT_EQ(0, hire_date);

    commit_transaction();
}

// Delete an un-inserted object with field values
TEST_F(gaia_object_test, new_get) {
    begin_transaction();
    
    auto e = new Employee();
    auto name = e->name_first();
    auto hire_date = e->hire_date();

    EXPECT_EQ(name, nullptr);
    EXPECT_EQ(0, hire_date);
    delete e;

    commit_transaction();
}

TEST_F(gaia_object_test, new_del_field_ref) {
    // create GAIA-64 scenario
    begin_transaction();

    auto e = new Employee();
    e->insert_row();
    e->delete_row();
    auto name = e->name_first();
    EXPECT_EQ(name, nullptr);
    auto hire_date = e->hire_date();
    EXPECT_EQ(hire_date, 0);
    e->set_name_last("Hendricks");

    commit_transaction();
}

TEST_F(gaia_object_test, new_del_original_field_ref) {
    // create GAIA-64 scenario
    begin_transaction();

    auto e = new Employee();
    e->insert_row();
    e->delete_row();
    auto name = e->name_first_original();
    EXPECT_EQ(name, nullptr);
    auto hire_date = e->hire_date_original();
    EXPECT_EQ(hire_date, 0);

    commit_transaction();
}

// Test on existing objects found by ID
// ====================================

// Create, write two rows, read back by ID and verify
TEST_F(gaia_object_test, read_back_id) {
    begin_transaction();
    auto eid = get_field("Howard")->gaia_id();
    auto eid2 = get_field("Henry")->gaia_id();
    commit_transaction();

    begin_transaction();
    auto e = Employee::get_row_by_id(eid);
    EXPECT_STREQ("Howard", e->name_first());
    EXPECT_STREQ("Howard", e->name_first_original());
    e = Employee::get_row_by_id(eid2);
    EXPECT_STREQ("Henry", e->name_first());
    EXPECT_STREQ("Henry", e->name_first_original());
    // Change the field and verify that original value is intact
    e->set_name_first("Heinrich");
    EXPECT_STREQ("Heinrich", e->name_first());
    EXPECT_STREQ("Henry", e->name_first_original());
    // While we have the original and modified values, update
    e->update_row();
    EXPECT_STREQ("Heinrich", e->name_first());
    EXPECT_STREQ("Henry", e->name_first_original());
    // Setting a field value after the update
    e->set_name_first("Hank");
    EXPECT_STREQ("Hank", e->name_first());
    EXPECT_STREQ("Henry", e->name_first_original());
    // Delete this object with original and modified fields
    e->delete_row();
    // The get, get_original and set should all success on this
    EXPECT_STREQ("Henry", e->name_first());
    EXPECT_STREQ("Henry", e->name_first_original());
    e->set_name_first("Harvey");
    // Finally, deleting this should be invalid
    EXPECT_THROW(e->delete_row(), invalid_node_id);
    commit_transaction();
    // Since e was deleted, this will not be cleaned up in TearDown
    delete e;
}

// Create row, try getting row from wrong type
TEST_F(gaia_object_test, read_wrong_type) {
    begin_transaction();
    auto eid = get_field("Howard")->gaia_id();
    commit_transaction();

    begin_transaction();
    try {
        Address::get_row_by_id(eid);
    }
    catch (const exception& e) {
        // The eid is unpredictable, but the exception will use it in its message.
        string compare_string = "Requesting Gaia type Address(2) but object identified by " + to_string(eid) + " is type Employee(1).";
        EXPECT_STREQ(e.what(), compare_string.c_str());
    }
    EXPECT_THROW(Address::get_row_by_id(eid), edc_invalid_object_type);
    commit_transaction();
}

// Create, write two rows, read back by scan and verify
TEST_F(gaia_object_test, read_back_scan) {
    begin_transaction();
    auto eid = get_field("Howard")->gaia_id();
    auto eid2 = get_field("Henry")->gaia_id();
    commit_transaction();

    begin_transaction();
    auto e = Employee::get_first();
    EXPECT_EQ(eid, e->gaia_id());
    EXPECT_STREQ("Howard", e->name_first());
    EXPECT_STREQ("Howard", e->name_first_original());
    e = e->get_next();
    EXPECT_EQ(eid2, e->gaia_id());
    EXPECT_STREQ("Henry", e->name_first());
    EXPECT_STREQ("Henry", e->name_first_original());
    commit_transaction();
}

// Used twice, below
void UpdateReadBack(bool update_flag) {
    begin_transaction();
    get_field("Howard");
    get_field("Henry");
    commit_transaction();

    begin_transaction();
    auto e = Employee::get_first();
    e->set_name_first("Herald");
    e->set_name_last("Hollman");
    if (update_flag) {
        e->update_row();
    }
    e = e->get_next();
    e->set_name_first("Gerald");
    e->set_name_last("Glickman");
    if (update_flag) {
        e->update_row();
    }
    commit_transaction();

    begin_transaction();
    e = Employee::get_first();
    if (update_flag) {
        EXPECT_STREQ("Herald", e->name_first());
        EXPECT_STREQ("Hollman", e->name_last());
    } else {
        // unchanged by previous transaction
        EXPECT_STREQ("Howard", e->name_first());
        EXPECT_STREQ(nullptr, e->name_last());
    }
    e = e->get_next();
    if (update_flag) {
        EXPECT_STREQ("Gerald", e->name_first());
        EXPECT_STREQ("Glickman", e->name_last());
    } else {
        // unchanged by previous transaction
        EXPECT_STREQ("Henry", e->name_first());
        EXPECT_STREQ(nullptr, e->name_last());
    }
    commit_transaction();
}

// Create, write two rows, set fields, update, read, verify
TEST_F(gaia_object_test, update_read_back) {
    UpdateReadBack(true);
}

// Create, write two rows, set fields, update, read, verify
TEST_F(gaia_object_test, no_update_read_back) {
    UpdateReadBack(false);
}

// Delete an inserted object then insert after, it's good again
TEST_F(gaia_object_test, new_del_ins) {
    begin_transaction();
    auto e = get_field("Hector");
    e->delete_row();
    e->insert_row();
    e->set_name_first("Hudson");
    if (e != nullptr) {
        delete e;
    }
    commit_transaction();
}

// Delete a found object then update
TEST_F(gaia_object_test, new_del_upd) {
    begin_transaction();
    auto e = get_field("Hector");
    e->delete_row();
    e->update_row();
    commit_transaction();
    delete e;
}

// Delete a found object then insert after, it's good again
TEST_F(gaia_object_test, found_del_ins) {
    begin_transaction();
    
    auto e = get_field("Hector");
    e->delete_row();
    e->insert_row();
    auto eid = e->gaia_id();
    commit_transaction();

    begin_transaction();
    e = Employee::get_row_by_id(eid);
    e->set_name_first("Hudson");
    if (e != nullptr) {
        delete e;
    }
    commit_transaction();

}

// Delete an inserted object then set field after, it's good again
TEST_F(gaia_object_test, new_del_set) {
    begin_transaction();
    auto e = get_field("Hector");
    e->delete_row();
    e->set_name_first("Howard");
    if (e != nullptr) {
        delete e;
    }
    commit_transaction();

}

// Delete a found object then update
TEST_F(gaia_object_test, found_del_upd) {
    begin_transaction();
    auto e = get_field("Hector");
    auto eid = e->gaia_id();
    commit_transaction();

    begin_transaction();
    e = Employee::get_row_by_id(eid);
    e->delete_row();
    e->update_row();
    if (e != nullptr) {
        delete e;
    }
    commit_transaction();

}

// EXCEPTION conditions
// ====================

// Attempt to insert one row twice
TEST_F(gaia_object_test, insert_x2) {
    begin_transaction();
    auto e = get_field("Zachary");
    EXPECT_THROW(e->insert_row(), duplicate_id);
    commit_transaction();
}

// Create, then get by ID, insert
// Used twice, below
void InsertIdX2(bool insert_flag) {
    begin_transaction();
    auto eid = get_field("Zachary")->gaia_id();
    commit_transaction();

    begin_transaction();
    auto e = Employee::get_row_by_id(eid);
    if (insert_flag) {
        e->set_name_first("Zerubbabel");
    }
    EXPECT_THROW(e->insert_row(), duplicate_id);
    commit_transaction();
}

// Attempt to insert a row found by ID
TEST_F(gaia_object_test, insert_id_x2) {
    InsertIdX2(false);
}

// Attempt to insert a row found by ID after setting field value
TEST_F(gaia_object_test, set_insert_id_x2) {
    InsertIdX2(true);
}

// Attempt to create a row outside of a transaction
TEST_F(gaia_object_test, no_tx) {
    EXPECT_THROW(get_field("Harold"), tx_not_open);
    // NOTE: the Employee object is leaked here
}

// Attempt to update an un-inserted object
TEST_F(gaia_object_test, new_upd) {
    begin_transaction();
    Employee* emp = new Employee();
    emp->set_name_first("Judith");
    EXPECT_THROW(emp->update_row(), invalid_node_id);
    if (emp != nullptr) {
        delete emp;
    }
    //Employee* emp2 = new Employee(0);
    //printf("%s\n", emp2->name_first());
    commit_transaction();
}

// Delete an un-inserted object
TEST_F(gaia_object_test, new_del) {
    begin_transaction();
    auto e = new Employee();
    EXPECT_THROW(e->delete_row(), invalid_node_id);
    if (e != nullptr) {
        delete e;
    }
    commit_transaction();
}

// Delete a row twice
TEST_F(gaia_object_test, new_del_del) {
    begin_transaction();
    auto e = get_field("Hugo");
    // the first delete succeeds
    e->delete_row();
    // second one fails
    EXPECT_THROW(e->delete_row(), invalid_node_id);
    commit_transaction();
    delete e;
}

// Perform get_next() without a preceeding get_first()
TEST_F(gaia_object_test, next_first) {
    begin_transaction();
    auto e1 = get_field("Harold");
    auto e2 = get_field("Howard");
    auto e3 = get_field("Hank");
    auto e_test = e2->get_next();
    EXPECT_TRUE(e_test == e1 || e_test == e3 || e_test == nullptr);
    auto e4 = new Employee();
    // In this case, the row doesn't exist yet.
    e4->set_name_first("Hector");
    EXPECT_EQ(nullptr, e4->get_next());
    commit_transaction();
}

void another_thread()
{
    begin_transaction();
    for (unique_ptr<Employee> e{Employee::get_first()}; e ; e.reset(e->get_next()))
    {
        EXPECT_TRUE(nullptr != e->name_first());
    }
    commit_transaction();
}

#include <thread>
TEST_F(gaia_object_test, thread_test) {
    begin_transaction();
    Employee::insert_row("Thread", "Master", "555-55-5555", 1234, "tid@tid.com", "www.thread.com");
    commit_transaction();
    // Run on this thread.
    another_thread();
    // Now spawn and run on another thread;
    thread t = thread(another_thread);
    t.join();
}
