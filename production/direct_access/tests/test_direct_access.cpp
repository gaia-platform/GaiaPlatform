/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <cstdlib>
#include <thread>

#include "gtest/gtest.h"
#include "addr_book_gaia_generated.h"
#include "db_test_helpers.hpp"

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
            e = e.get_first())
        {
            e.delete_row();
        }
        commit_transaction();
    }

    static void SetUpTestSuite() {
        start_server();
    }

    static void TearDownTestSuite() {
        stop_server();
    }

    void SetUp() override {
        begin_session();
        delete_employees();
    }

    void TearDown() override {
        delete_employees();
        end_session();
    }
};

int count_rows() {
    int count = 0;
    for (auto e = Employee::get_first();
         e;
         e = e.get_next())
    {
        ++count;
    }
    return count;
}

Employee_writer set_field(const char* name) {
    auto w = Employee_writer();
    w.name_first = name;
    return w;
}

Employee insert_row(const char* name) {
    auto w = set_field(name);
    gaia_id_t id = w.insert_row();
    return Employee::get(id);
}

Employee update_row(const char* name) {
    auto e = insert_row(name);
    e.writer().update_row();
    return e;
}

Employee get_field(const char* name) {
    auto e = update_row(name);
    EXPECT_STREQ(e.name_first(), name);
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
    e.delete_row();
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
    auto w = Employee_writer();
    w.name_last = "Smith";
    EXPECT_STREQ(w.name_last.c_str(), "Smith");
}

// Attempt to read original value from new object
TEST_F(gaia_object_test, read_original_from_copy) {
    begin_transaction();
    auto e = get_field("Zachary");
    EXPECT_STREQ("Zachary", e.name_first());
    commit_transaction();
}

// Insert a row with no field values
TEST_F(gaia_object_test, new_ins_get) {
    begin_transaction();

    auto e = Employee::get(Employee_writer().insert_row());
    auto name = e.name_first();
    auto hire_date = e.hire_date();

    EXPECT_EQ(name, nullptr);
    EXPECT_EQ(0, hire_date);

    commit_transaction();
}

// Read values from a non-inserted writer
TEST_F(gaia_object_test, new_get) {
    begin_transaction();
    auto w = Employee_writer();
    auto name = w.name_first;
    auto hire_date = w.hire_date;

    EXPECT_TRUE(name.empty());
    EXPECT_EQ(0, hire_date);
    commit_transaction();
}

TEST_F(gaia_object_test, new_del_field_ref) {
    // create GAIA-64 scenario
    begin_transaction();

    auto e = Employee::get(Employee_writer().insert_row());
    e.delete_row();
    // can't access data from a deleted row
    EXPECT_THROW(e.name_first(), invalid_node_id);

    // Can't get a writer from a deleted row either
    EXPECT_THROW(e.writer(), invalid_node_id);

    commit_transaction();
}

// Attempt to update a row with a new writer.
// This test doesn't make sense any more since you can't create a new employee
// object but only a new writer.

/*
TEST_F(gaia_object_test, new_upd_field) {
    begin_transaction();
    auto writer = Employee_writer();
    EXPECT_THROW(Employee::update_row(writer), invalid_node_id);
    commit_transaction();
}
*/

// Attempt to insert with an update writer, this should work.
TEST_F(gaia_object_test, existing_ins_field) {
    begin_transaction();
    auto e = Employee::get(Employee_writer().insert_row());
    auto writer = e.writer();
    writer.insert_row();
    commit_transaction();
}


// Test on existing objects found by ID
// ====================================

// Create, write two rows, read back by ID and verify
TEST_F(gaia_object_test, read_back_id) {
    begin_transaction();
    auto eid = get_field("Howard").gaia_id();
    auto eid2 = get_field("Henry").gaia_id();
    commit_transaction();

    begin_transaction();
    auto e = Employee::get(eid);
    EXPECT_STREQ("Howard", e.name_first());
    e = Employee::get(eid2);
    EXPECT_STREQ("Henry", e.name_first());
    // Change the field and verify that original value is intact
    auto writer = e.writer();
    writer.name_first = "Heinrich";
    EXPECT_STREQ("Henry", e.name_first());
    // While we have the original and modified values, update
    writer.update_row();
    EXPECT_STREQ("Heinrich", e.name_first());
    EXPECT_STREQ("Heinrich", writer.name_first.c_str());

    // Setting a field value after the update
    writer.name_first = "Hank";
    EXPECT_STREQ("Heinrich", e.name_first());
    // Delete this object with original and modified fields
    e.delete_row();
    // Can't access data of a deleted row
    EXPECT_THROW(e.name_first(), invalid_node_id);
    commit_transaction();
}

// Create row, try getting row from wrong type
TEST_F(gaia_object_test, read_wrong_type) {
    begin_transaction();
    auto e = get_field("Howard");
    auto eid = e.gaia_id();
    commit_transaction();

    begin_transaction();
    try {
        auto a = Address::get(eid);
        gaia_id_t id = a.gaia_id();
        EXPECT_EQ(id, 1000);
    }
    catch (const exception& e) {
        // The eid is unpredictable, but the exception will use it in its message.
        string compare_string = "Requesting Gaia type Address(2) but object identified by " + to_string(eid) + " is type (1).";
        EXPECT_STREQ(e.what(), compare_string.c_str());
    }
    EXPECT_THROW(Address::get(eid), edc_invalid_object_type);
    commit_transaction();
}

// Create, write two rows, read back by scan and verify
TEST_F(gaia_object_test, read_back_scan) {
    begin_transaction();
    auto eid = get_field("Howard").gaia_id();
    auto eid2 = get_field("Henry").gaia_id();
    commit_transaction();

    begin_transaction();
    auto e = Employee::get_first();
    EXPECT_EQ(eid, e.gaia_id());
    EXPECT_STREQ("Howard", e.name_first());
    e = e.get_next();
    EXPECT_EQ(eid2, e.gaia_id());
    EXPECT_STREQ("Henry", e.name_first());
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
    auto w = e.writer();
    w.name_first = "Herald";
    w.name_last = "Hollman";
    if (update_flag) {
        w.update_row();
    }
    e = e.get_next();

    // get writer for next row!
    w = e.writer();
    w.name_first = "Gerald";
    w.name_last = "Glickman";
    if (update_flag) {
        w.update_row();
    }
    commit_transaction();

    begin_transaction();
    e = Employee::get_first();
    if (update_flag) {
        EXPECT_STREQ("Herald", e.name_first());
        EXPECT_STREQ("Hollman", e.name_last());
    } else {
        // unchanged by previous transaction
        EXPECT_STREQ("Howard", e.name_first());
        EXPECT_STREQ(nullptr, e.name_last());
    }
    e = e.get_next();
    if (update_flag) {
        EXPECT_STREQ("Gerald", e.name_first());
        EXPECT_STREQ("Glickman", e.name_last());
    } else {
        // unchanged by previous transaction
        EXPECT_STREQ("Henry", e.name_first());
        EXPECT_STREQ(nullptr, e.name_last());
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

// Delete an inserted object then insert after; the new row is good.
TEST_F(gaia_object_test, new_del_ins) {
    begin_transaction();
    auto e = get_field("Hector");
    auto w = e.writer();
    e.delete_row();
    w.insert_row();
    commit_transaction();
}

// Delete a found object then update
TEST_F(gaia_object_test, new_del_upd) {
    begin_transaction();
    auto e = get_field("Hector");
    e.delete_row();
    EXPECT_THROW(e.writer().update_row(), invalid_node_id);
    commit_transaction();
}

// Delete a found object then insert after, it's good again.
TEST_F(gaia_object_test, found_del_ins) {
    begin_transaction();

    auto e = get_field("Hector");
    auto writer = e.writer();
    e.delete_row();
    EXPECT_THROW(e.writer(), invalid_node_id);
    // We got the writer before we deleted the row.
    // We can't update the row but we can insert a new one.
    auto eid = writer.insert_row();
    commit_transaction();

    begin_transaction();
    e = Employee::get(eid);
    e.writer().name_first = "Hudson";
    commit_transaction();
}

// Delete a found object then update
TEST_F(gaia_object_test, found_del_upd) {
    begin_transaction();
    auto e = get_field("Hector");
    auto eid = e.gaia_id();
    commit_transaction();

    begin_transaction();
    e = Employee::get(eid);
    auto w = e.writer();
    e.delete_row();
    EXPECT_THROW(w.update_row(), invalid_node_id);
    commit_transaction();
}

// EXCEPTION conditions
// ====================

// The simplified model allows you to
// do multiple insertions with the same
// writer.  This seems reasonable given the model
// of passing in the row object to an insert_row method
// auto writer = Employe::writer();
// writer.insert_row();
// writer.insert_row();

/*
// Attempt to insert one row twice
TEST_F(gaia_object_test, insert_x2) {
    begin_transaction();
    auto e = get_field("Zachary");
    EXPECT_THROW(e.insert_row(), duplicate_id);
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
        e.set_name_first("Zerubbabel");
    }
    EXPECT_THROW(e.insert_row(), duplicate_id);
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
// you cannot create a new Employee() and the
// Employee::writer method returns an Employee_writer
// not an Employee object.  The update and delete
// methods require a reference to an Employee object
// and not a writer object.  The insert method
// only takes a writer object.

/*

// Attempt to update an un-inserted object
TEST_F(gaia_object_test, new_upd) {
    begin_transaction();
    auto emp = Employee_writer();
    emp->set_name_first("Judith");
    EXPECT_THROW(emp->update_row(), invalid_node_id);
    //Employee* emp2 = new Employee(0);
    //printf("%s\n", emp2->name_first());
    commit_transaction();
}

// Delete an un-inserted object
TEST_F(gaia_object_test, new_del) {
    begin_transaction();
    auto e = Employee_writer();
    EXPECT_THROW(e.delete_row(), invalid_node_id);
    commit_transaction();
}
*/

// Delete a row twice
TEST_F(gaia_object_test, new_del_del) {
    begin_transaction();
    auto e = get_field("Hugo");
    // The first delete succeeds.
    e.delete_row();
    // The second one should throw.
    EXPECT_THROW(e.delete_row(), invalid_node_id);
    commit_transaction();
}

// Simplified model doesn't allow this scenario because 
// Employee_writer() returns a writer and not an Employee()
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
    auto e4 = Employee_writer();
    // In this case, the row doesn't exist yet.
    e4->set_name_first("Hector");
    EXPECT_EQ(nullptr, e4->get_next());
    commit_transaction();
}
*/

TEST_F(gaia_object_test, auto_tx_begin) {

    // Default constructor enables auto_begin semantics
    auto_transaction_t tx;

    auto writer = Employee_writer();
    writer.name_last = "Hawkins";
    Employee e = Employee::get(writer.insert_row());
    tx.commit();

    EXPECT_STREQ(e.name_last(), "Hawkins");
    
    // update
    writer = e.writer();
    writer.name_last = "Clinton";
    writer.update_row();
    tx.commit();
    
    EXPECT_STREQ(e.name_last(), "Clinton");
}

TEST_F(gaia_object_test, auto_tx) {
    // Specify auto_begin = false
    auto_transaction_t tx(false);
    auto writer = Employee_writer();

    writer.name_last = "Hawkins";
    Employee e = Employee::get(writer.insert_row());
    tx.commit();

    // Expect an exception since we're not in a transaction
    EXPECT_THROW(e.name_last(), tx_not_open);

    begin_transaction();

    EXPECT_STREQ(e.name_last(), "Hawkins");

    // This is legal.
    tx.commit();
}

TEST_F(gaia_object_test, auto_tx_rollback) {
    gaia_id_t id;
    {
        auto_transaction_t tx;
        auto writer = Employee_writer();
        writer.name_last = "Hawkins";
        id = writer.insert_row();
    }
    // Transaction was rolled back 
    auto_transaction_t tx;
    Employee e = Employee::get(id);
    EXPECT_FALSE(e);
}


TEST_F(gaia_object_test, writer_value_ref) {
    begin_transaction();
    Employee_writer w1 = Employee_writer();
    w1.name_last = "Gretzky";
    Employee e = Employee::get(w1.insert_row());
    commit_transaction();

    begin_transaction();
    w1 = e.writer();
    auto& ssn = w1.ssn;
    ssn = "987654321";
    w1.update_row();
    commit_transaction();

    begin_transaction();
    EXPECT_STREQ(e.ssn(), "987654321");
    commit_transaction();
}

const char* g_insert = "insert_thread";
gaia_id_t g_inserted_id = INVALID_GAIA_ID;
void insert_thread(bool new_thread)
{
    if (new_thread) { begin_session(); }
    begin_transaction();
        g_inserted_id = Employee::insert_row(g_insert, nullptr, nullptr, 0, nullptr, nullptr);
    commit_transaction();
    if (new_thread) { end_session(); }
}

const char* g_update = "update_thread";
void update_thread(gaia_id_t id)
{
    begin_session();
    begin_transaction();
        Employee e = Employee::get(id);
        Employee_writer w = e.writer();
        w.name_first = g_update;
        w.update_row();
    commit_transaction();
    end_session();
}

void delete_thread(gaia_id_t id)
{
    begin_session();
    begin_transaction();
        Employee::delete_row(id);
    commit_transaction();
    end_session();
}

TEST_F(gaia_object_test, thread_insert) {
    // Insert a record in another thread and verify
    // we can see it here.
    thread t = thread(insert_thread, true);
    t.join();
    begin_transaction();
        Employee e = Employee::get(g_inserted_id);
        EXPECT_STREQ(e.name_first(), g_insert);
    commit_transaction();
}

TEST_F(gaia_object_test, thread_update) {
    // Update a record in another thread and verify
    // we can see it here.
    insert_thread(false);
    
    begin_transaction();
        Employee e = Employee::get(g_inserted_id);
        EXPECT_STREQ(e.name_first(), g_insert);

        // Update the same record in a different transaction and commit.
        thread t = thread(update_thread, g_inserted_id);
        t.join();

        // The change should not be visible in our transaction
        EXPECT_STREQ(e.name_first(), g_insert);
    commit_transaction();

    begin_transaction();
        // now we should have the new value
        EXPECT_STREQ(e.name_first(), g_update);
    commit_transaction();
}

TEST_F(gaia_object_test, thread_update_conflict) {
    insert_thread(false);
    
    begin_transaction();
        // Update the same record in a different transaction and commit
        thread t = thread(update_thread, g_inserted_id);
        t.join();

        Employee_writer w = Employee::get(g_inserted_id).writer();
        w.name_first = "Violation";
        w.update_row();

    // Expect a concurrency violation here, but for now commit_transaction is
    // returning false.
    // EXPECT_THROW(commit_transaction, tx_update_conflict);
    EXPECT_FALSE(commit_transaction());

    begin_transaction();
        // Actual value here is g_update, which shows that my update never
        // went through.
        EXPECT_STREQ(Employee::get(g_inserted_id).name_first(), g_update);
    commit_transaction();
}

TEST_F(gaia_object_test, thread_update_other_row) {
    begin_transaction();
        gaia_id_t row1_id = Employee::insert_row(g_insert, nullptr, nullptr, 0, nullptr, nullptr);
        gaia_id_t row2_id = Employee::insert_row(g_insert, nullptr, nullptr, 0, nullptr, nullptr);
    commit_transaction();

    begin_transaction();
        // Update the same record in a different transaction and commit
        thread t = thread(update_thread, row1_id);
        t.join();

        Employee_writer w = Employee::get(row2_id).writer();
        w.name_first = "No Violation";
        w.update_row();
    EXPECT_TRUE(commit_transaction());

    begin_transaction();
        // Row 1 should have been updated by the update thread.
        // Row 2 should have been updated by this thread.
        EXPECT_STREQ(Employee::get(row1_id).name_first(), g_update);
        EXPECT_STREQ(Employee::get(row2_id).name_first(), "No Violation");
    commit_transaction();
}

TEST_F(gaia_object_test, thread_delete) {
    // update a record in another thread and verify
    // we can see it 
    insert_thread(false);
    begin_transaction();
        // Delete the record we just inserted
        thread t = thread(delete_thread, g_inserted_id);
        t.join();

        // The change should not be visible in our transaction and
        // we should be able to access the record just fine.
        EXPECT_STREQ(Employee::get(g_inserted_id).name_first(), g_insert);
    commit_transaction();

    begin_transaction();
        // Now this should fail.
        EXPECT_THROW(Employee::get(g_inserted_id).name_first(), invalid_node_id);
    commit_transaction();
}

TEST_F(gaia_object_test, thread_insert_update_delete) {
    // Do three concurrent operations and make sure we see are isolated from them
    // and then do see them in a subsequent transaction.
    const char* local = "My Insert";
    begin_transaction();
        gaia_id_t row1_id = Employee::insert_row(local, nullptr, nullptr, 0, nullptr, nullptr);
        gaia_id_t row2_id = Employee::insert_row("Red Shirt", nullptr, nullptr, 0, nullptr, nullptr);
    commit_transaction();

    begin_transaction();
        thread t1 = thread(delete_thread, row2_id);
        thread t2 = thread(insert_thread, true);
        thread t3 = thread(update_thread, row1_id);
        t1.join();
        t2.join();
        t3.join();
        EXPECT_STREQ(Employee::get(row1_id).name_first(), local);
        EXPECT_STREQ(Employee::get(row2_id).name_first(), "Red Shirt");
    commit_transaction();

    begin_transaction();
        // Deleted row2.
        EXPECT_THROW(Employee::get(row2_id).name_first(), invalid_node_id);
        // Inserted a new row
        EXPECT_STREQ(Employee::get(g_inserted_id).name_first(), g_insert);
        // Updated row1.
        EXPECT_STREQ(Employee::get(row1_id).name_first(), g_update);
    commit_transaction();
};

TEST_F(gaia_object_test, thread_delete_conflict) {
    // Have two threads delete the same row.
    insert_thread(false);
    begin_transaction();
        Employee::delete_row(g_inserted_id);
        thread t1 = thread(delete_thread, g_inserted_id);
        t1.join();
    // Expect a concurrency violation here, but for now commit_transaction is
    // returning false.
    // EXPECT_THROW(commit_transaction, tx_update_conflict);
    EXPECT_FALSE(commit_transaction());

    begin_transaction();
        // Expect the row to be deleted so another attempt to delete should fail.
        EXPECT_THROW(Employee::delete_row(g_inserted_id), invalid_node_id);
    commit_transaction();
};
