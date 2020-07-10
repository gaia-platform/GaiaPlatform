/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include "gtest/gtest.h"
#include "addr_book_gaia_generated.h"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;
using namespace AddrBook;


class gaia_object_test : public ::testing::Test {
protected:
    void delete_employees() {
        begin_transaction();
        for(auto e = Employee::get_first();
            e;
            e = e->get_next())
        {
            e->delete_row();
        }
        commit_transaction();
        gaia_base_t::s_gaia_cache.clear();
    }

    void SetUp() override {
        gaia_mem_base::init(true);
    }

    void TearDown() override {
        delete_employees();
        // Delete the shared memory segments.
        gaia_mem_base::reset();
    }
};

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

Employee_writer set_field(const char* name) {
    auto e = Employee::writer();
    e->name_first = name;
    return e;
}

Employee_ptr insert_row(const char* name) {
    auto e = set_field(name);
    EXPECT_NE(e, nullptr);
    gaia_id_t id = Employee::insert_row(e);
    return Employee::get(id);
}

Employee_ptr update_row(const char* name) {
    auto e = insert_row(name);
    EXPECT_NE(e, nullptr);
    auto w = Employee::writer(e);
    Employee::update_row(w);
    return e;
}

Employee_ptr get_field(const char* name) {
    auto e = update_row(name);
    EXPECT_STREQ(e->name_first(), name);
    return e;
}

// Test on objects created by new()
// ================================

// Create, write & read, one row
TEST_F(gaia_object_test, get_field) {
    begin_transaction();
    get_field("Harold");
    commit_transaction();
}

// Delete one row
TEST_F(gaia_object_test, get_field_delete) {
    begin_transaction();
    auto e = get_field("Jameson");
    e->delete_row();
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
    auto e = Employee::writer();
    e->name_last = "Smith";
    EXPECT_STREQ(e->name_last.c_str(), "Smith");
}

// Attempt to read original value from new object
TEST_F(gaia_object_test, read_original_from_copy) {
    begin_transaction();
    auto e = get_field("Zachary");
    EXPECT_STREQ("Zachary", e->name_first());
    commit_transaction();
}

// Insert a row with no field values
TEST_F(gaia_object_test, new_ins_get) {
    begin_transaction();
    
    auto writer = Employee::writer();
    Employee::insert_row(writer);
    auto e = Employee::get(Employee::insert_row(writer));
    auto name = e->name_first();
    auto hire_date = e->hire_date();

    EXPECT_EQ(name, nullptr);
    EXPECT_EQ(0, hire_date);

    commit_transaction();
}

// Delete an un-inserted object with field values
TEST_F(gaia_object_test, new_get) {
    begin_transaction();
    
    auto e = Employee::writer();
    auto name = e->name_first;
    auto hire_date = e->hire_date;

    EXPECT_TRUE(name.empty());
    EXPECT_EQ(0, hire_date);
    commit_transaction();
}

TEST_F(gaia_object_test, new_del_field_ref) {
    // create GAIA-64 scenario
    begin_transaction();

    auto writer = Employee::writer();
    auto e = Employee::get(Employee::insert_row(writer));
    e->delete_row();
    auto name = e->name_first();
    EXPECT_EQ(name, nullptr);
    auto hire_date = e->hire_date();
    EXPECT_EQ(hire_date, 0);
    writer = Employee::writer(e);
    writer->name_last = "Hendricks";

    commit_transaction();
}

// Attempt to update a row with a new writer.
TEST_F(gaia_object_test, new_upd_field) {
    begin_transaction();
    auto writer = Employee::writer();
    EXPECT_THROW(Employee::update_row(writer), invalid_node_id);
    commit_transaction();
}

// Attempt to insert with an update writer
TEST_F(gaia_object_test, existing_ins_field) {
    begin_transaction();
    auto writer = Employee::writer();
    auto e = Employee::get(Employee::insert_row(writer));
    writer = Employee::writer(e);
    EXPECT_THROW(Employee::insert_row(writer), invalid_node_id);
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
    auto e = Employee::get(eid);
    EXPECT_STREQ("Howard", e->name_first());
    e = Employee::get(eid2);
    EXPECT_STREQ("Henry", e->name_first());
    // Change the field and verify that original value is intact
    auto writer = Employee::writer(e);
    writer->name_first = "Heinrich";
    EXPECT_STREQ("Henry", e->name_first());
    // While we have the original and modified values, update
    Employee::update_row(writer);
    EXPECT_STREQ("Heinrich", e->name_first());
    EXPECT_STREQ("Heinrich", writer->name_first.c_str());

    // Setting a field value after the update
    writer->name_first = "Hank";
    EXPECT_STREQ("Heinrich", e->name_first());
    // Delete this object with original and modified fields
    e->delete_row();
    // The get, get_original and set should all success on this
    EXPECT_STREQ("Heinrich", e->name_first());
    writer->name_first = "Harvey";
    // Finally, deleting this should be invalid
    EXPECT_THROW(e->delete_row(), invalid_node_id);
    commit_transaction();
}

// Create row, try getting row from wrong type
TEST_F(gaia_object_test, read_wrong_type) {
    begin_transaction();
    auto e = get_field("Howard");
    auto eid = e->gaia_id();
    commit_transaction();

    begin_transaction();
    try {
        auto a = Address::get(eid);
        gaia_id_t id = a->gaia_id();
        EXPECT_EQ(id, 1000);
    }
    catch (const exception& e) {
        // The eid is unpredictable, but the exception will use it in its message.
        string compare_string = "Requesting Gaia type Address(2) but object identified by " + to_string(eid) + " is type Employee(1).";
        EXPECT_STREQ(e.what(), compare_string.c_str());
    }
    EXPECT_THROW(Address::get(eid), edc_invalid_object_type);
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
    e = e->get_next();
    EXPECT_EQ(eid2, e->gaia_id());
    EXPECT_STREQ("Henry", e->name_first());
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
    auto w = Employee::writer(e);
    w->name_first = "Herald";
    w->name_last = "Hollman";
    if (update_flag) {
        Employee::update_row(w);
    }
    e = e->get_next();

    // get writer for next row!
    w = Employee::writer(e);
    w->name_first = "Gerald";
    w->name_last = "Glickman";
    if (update_flag) {
        Employee::update_row(w);
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
    
    auto w = Employee::writer(e);
    e->delete_row();
    EXPECT_THROW(Employee::insert_row(w), invalid_node_id);
    commit_transaction();
}

// Delete a found object then update
TEST_F(gaia_object_test, new_del_upd) {
    begin_transaction();
    auto e = get_field("Hector");
    e->delete_row();
    auto w = Employee::writer(e);
    EXPECT_THROW(Employee::update_row(w), invalid_node_id);
    commit_transaction();
}

// Delete a found object then insert after, it's good again
TEST_F(gaia_object_test, found_del_ins) {
    begin_transaction();
    
    auto e = get_field("Hector");
    e->delete_row();
    Employee_writer writer = Employee::writer();
    auto eid = Employee::insert_row(writer);
    commit_transaction();

    begin_transaction();
    e = Employee::get(eid);
    writer = Employee::writer(e);
    writer->name_first = "Hudson";
    commit_transaction();
}

// Delete an inserted object then set field after, it's good again
TEST_F(gaia_object_test, new_del_set) {
    begin_transaction();
    auto e = get_field("Hector");
    e->delete_row();
    auto w = Employee::writer(e);
    w->name_first = "Howard";
    commit_transaction();
}

// Delete a found object then update
TEST_F(gaia_object_test, found_del_upd) {
    begin_transaction();
    auto e = get_field("Hector");
    auto eid = e->gaia_id();
    commit_transaction();

    begin_transaction();
    e = Employee::get(eid);
    e->delete_row();
    auto w = Employee::writer(e);
    EXPECT_THROW(Employee::update_row(w), invalid_node_id);
    commit_transaction();
}

// EXCEPTION conditions
// ====================

// The simplified model allows you to
// do multiple insertions with the same
// writer.  This seems reasonable given the model
// of passing in the row object to an insert_row method
// auto writer = Employe::writer();
// Employe::insert_row(writer);
// Employe::insert_row(writer); // this is fine.

/*

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
    auto e = Employee::get(eid);
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
*/

// Attempt to create a row outside of a transaction
TEST_F(gaia_object_test, no_tx) {
    EXPECT_THROW(get_field("Harold"), tx_not_open);
    // NOTE: the Employee object is leaked here
}

// Simplified model doesn't allow this because you
// you cannot create a new Employee() and you the
// Employee::create method returns an Employee_writer
// not an Employee object.  The update and delete
// methods require a reference to an Employee object
// and not a writer object.  The insert method
// only takes a writer object.

/*

// Attempt to update an un-inserted object
TEST_F(gaia_object_test, new_upd) {
    begin_transaction();
    auto emp = Employee::writer();
    emp->set_name_first("Judith");
    EXPECT_THROW(emp->update_row(), invalid_node_id);
    //Employee* emp2 = new Employee(0);
    //printf("%s\n", emp2->name_first());
    commit_transaction();
}

// Delete an un-inserted object
TEST_F(gaia_object_test, new_del) {
    begin_transaction();
    auto e = Employee::writer();
    EXPECT_THROW(e->delete_row(), invalid_node_id);
    commit_transaction();
}
*/

// Delete a row twice
TEST_F(gaia_object_test, new_del_del) {
    begin_transaction();
    auto e = get_field("Hugo");
    // the first delete succeeds
    e->delete_row();
    // second one fails
    EXPECT_THROW(e->delete_row(), invalid_node_id);
    commit_transaction();
}

// Simplified model doesn't allow this scenario because 
// Employee::writer() returns a writer and not an Employee()
// object.

// Perform get_next() without a preceeding get_first()

/*
TEST_F(gaia_object_test, next_first) {
    begin_transaction();
    auto e1 = get_field("Harold");
    auto e2 = get_field("Howard");
    auto e3 = get_field("Hank");
    auto e_test = e2->get_next();
    EXPECT_TRUE(e_test == e1 || e_test == e3 || e_test == nullptr);
    auto e4 = Employee::writer();
    // In this case, the row doesn't exist yet.
    e4->set_name_first("Hector");
    EXPECT_EQ(nullptr, e4->get_next());
    commit_transaction();
}
*/

// If tx is non-null then test implicit and auto-begin
// transactions.  Otherwise, use the same model
// The two should have the exact same semantics.
TEST_F(gaia_object_test, auto_tx) {
    auto_transaction_t tx;

    auto writer = Employee::writer();
    EXPECT_EQ(gaia_base_t::s_gaia_cache.size(), 0);

    writer->name_last = "Hawkins";
    Employee_ptr e = Employee::get(Employee::insert_row(writer));
    tx.commit();

    EXPECT_STREQ(e->name_last(), "Hawkins");
    EXPECT_EQ(gaia_base_t::s_gaia_cache.size(), 1);
    
    // update
    writer = Employee::writer(e);
    writer->name_last = "Clinton";
    Employee::update_row(writer);
    tx.commit();
    
    EXPECT_EQ(gaia_base_t::s_gaia_cache.size(), 1);
    EXPECT_STREQ(e->name_last(), "Clinton");


    // Ensure scoping is fine; we should be able to insert a row
    // should also not work.  Again not an issue
    // in the non-simplified API because there is
    // no object off of which youc an call update_row()
    Address_ptr a;
    {
        auto a_writer = Address::writer();
        EXPECT_EQ(gaia_base_t::s_gaia_cache.size(), 1);
        a_writer->city = "Seattle";
        a = Address::get(Address::insert_row(a_writer));
        tx.commit();
        a_writer = Address::writer(a);
        a_writer->city = "Portland";
        EXPECT_EQ(gaia_base_t::s_gaia_cache.size(), 2);
    }
    tx.commit();
    EXPECT_STREQ(a->city(), "Seattle");
}

void another_thread()
{
    begin_transaction();
    for (auto e = Employee::get_first(); e ; e =e->get_next())
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

/*
TEST_F(gaia_object_test, simplified_api) {

    // insert row, m_num_references should be schema
    EmployeeWriter& w = Employee::writer();
    w.last_name = "Hawkins";
    //w.insert_row();
    Employee& e = Employee::get(Employee::insert_row(w));

    EmployeeWriter& w = e.writer();
    Employee::update_row(w);

    // nested class
    Employee::Writer::insert(w);
    Employee::Writer::update(w);
    

}
*/
