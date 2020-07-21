/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <cstdlib>

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
            e = e.get_next())
        {
            e.delete_row();
        }
        commit_transaction();
    }

    void SetUp() override {
        start_server();
        // Start new session with server.
        begin_session();
    }

    void TearDown() override {
        delete_employees();
        end_session();
        stop_server();
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
// Employe::insert_row(writer);
// Employe::insert_row(writer); // this is fine.

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
    // the first delete succeeds
    e.delete_row();
    // second one is a no-op
    e.delete_row();
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

void another_thread()
{
    begin_transaction();
    for (auto e = Employee::get_first(); e ; e =e.get_next())
    {
        EXPECT_TRUE(nullptr != e.name_first());
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
TEST_F(gaia_object_test, writer_tests) {
    // A new writer is not shared so it's use count is always
    // equal to the number of outstanding references.
    Employee_writer w1 = Employee_writer();
    EXPECT_EQ(1, w1.use_count());
    {
        Employee_writer w2 = Employee_writer();
        EXPECT_EQ(1, w1.use_count());
        EXPECT_EQ(1, w2.use_count());
    }
    EXPECT_EQ(1, w1.use_count());
    Employee_writer w2 = w1;
    EXPECT_EQ(2, w1.use_count());
    EXPECT_EQ(2, w2.use_count());
}

TEST_F(gaia_object_test, writer_tests) {
    begin_transaction();
    // An existing writer is shared with the owning
    // gaia object.
    Employee_writer w1 = Employee_writer();
    w1->name_last = "Gretzky";
    Employee_ptr e = Employee::get(Employee::insert_row(w1));
    w1.reset();
    EXPECT_EQ(0, w1.use_count());

    // Verify that the writer returned from writer and writer methods
    // is the same type.  If they are not you'll get a compile error here.
    w1 = e->writer();

    // The owning employee object is holding on to a reference here so
    // we expect 2.
    EXPECT_EQ(2, w1.use_count());
    w1->name_first = "Wayne";
    w1.reset();

    // We just reset the shared pointer here so it's use_count should be 0.
    EXPECT_EQ(0, w1.use_count());

    // Because the employee object had a reference to the writer, however,
    // we haven't lost our changes above.
    EXPECT_STREQ(e->writer()->name_first.c_str(), "Wayne");

    // Now make some other changes without having an explicit reference.
    e->writer()->name_last = "Gretzky";
    e->writer()->ssn = "123456789";
    e->update_row();

    // Verify we've persisted the values and updated the underlying storage.
    EXPECT_STREQ(e->name_first(), "Wayne");
    EXPECT_STREQ(e->name_last(), "Gretzky");
    EXPECT_STREQ(e->ssn(), "123456789");

    // Verify the writer has these same values as well and still only
    // has a ref count of 2.
    w1 = e->writer();
    EXPECT_EQ(2, w1.use_count());

    EXPECT_STREQ(w1->name_first.c_str(), "Wayne");
    EXPECT_STREQ(w1->name_last.c_str(), "Gretzky");
    EXPECT_STREQ(w1->ssn.c_str(), "123456789");
    commit_transaction();

    // Our reference count should still be two because we are holding on
    // to w1 and e.
    EXPECT_EQ(2, w1.use_count());

    // Drop our reference to the employee.
    e.reset();

    // Writer reference count should still be two.  The gaia cache is holding a 
    // reference to e which is holding a reference to the writer.
    EXPECT_EQ(2, w1.use_count());

    // Since there aren't any outstanding references to the employee, the object
    // gets dropped from the gaia object cache on our next transaction.
    begin_transaction();

    // Since the cache has dropped employee, it will have dropped its reference
    // to the writer and we are down to 1.  The writer should still be valid
    // to reference.
    EXPECT_EQ(1, w1.use_count());
    EXPECT_STREQ(w1->name_first.c_str(), "Wayne");

    // However, it is now detached from the employee.
    w1->name_last = "Wilson";
    EXPECT_STREQ(w1->name_last.c_str(), "Wilson");

    e = Employee::get_first();
    EXPECT_STREQ(e->name_first(), "Wayne");
    EXPECT_STREQ(e->name_last(), "Gretzky");
    EXPECT_STREQ(e->ssn(), "123456789");

    // The writer is detached at this point so this call is a no-op.
    e->update_row();
    EXPECT_STREQ(e->name_first(), "Wayne");
    EXPECT_STREQ(e->name_last(), "Gretzky");
    EXPECT_STREQ(e->ssn(), "123456789");
    commit_transaction();
}

TEST_F(gaia_object_test, writer_value_ref) {
    begin_transaction();
    Employee_writer w1 = Employee_writer();
    w1->name_last = "Gretzky";
    Employee_ptr e = Employee::get(Employee::insert_row(w1));
    commit_transaction();

    begin_transaction();
    auto& ssn = e->writer()->ssn;
    ssn = "987654321";
    e->update_row();
    commit_transaction();

    begin_transaction();
    EXPECT_STREQ(e->ssn(), "987654321");
    commit_transaction();
}

TEST_F(gaia_object_test, writer_api)
{
    begin_transaction();
    // pre-populated
    auto writer = Employee::get_writer(Employee& e);
    writer.last_name() = foo;
    writer.first_name() = bar;
    writer->insert_row();
    writer->update_row();


    // how would you code:
    // x.last_name = "hawkins";
    // stuff happens here
    // x.first_name = "dax;"
    {
        auto writer = Employee::get_writer(e);
        writer.last_name = "hawkins";
        writer.first_name = "dax";
        writer->update_row();
    }

    {auto writer = Employee::get_writer(Employee& e); writer->last_name="hawkins"; writer->update_row();}
    {auto writer = Employee::get_writer(Employee& e); writer->last_name="hawkins"; writer->update_row();}

    auto writer =
}
*/
