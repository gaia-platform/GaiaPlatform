/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <cstdlib>
#include <thread>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "gtest/gtest.h"
#include "gaia_addr_book.h"
#include "db_test_base.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::addr_book;


class gaia_object_test : public db_test_base_t {
};

int count_rows() {
    // Old method for scanning objects:
    int cnt = 0;
    for (auto e = employee_t::get_first(); e; e = e.get_next()) {
        ++cnt;
    }

    // Primary method for scanning objects:
    int count = 0;
    for (auto e : employee_t::list()) {
        ++count;
    }
    EXPECT_EQ(count,cnt);
    return count;
}

// Utility function that creates one named employee row.
employee_t create_employee(const char* name) {
    auto w = employee_writer();
    w.name_first = name;
    gaia_id_t id = w.insert_row();
    auto e = employee_t::get(id);
    EXPECT_STREQ(e.name_first(), name);
    return e;
}

// Test on objects created by new()
// ================================

// Create, write & read, one row
TEST_F(gaia_object_test, create_employee) {
    begin_transaction();
    create_employee("Harold");
    commit_transaction();
}

// Delete one row
TEST_F(gaia_object_test, create_employee_delete) {
    begin_transaction();
    auto e = create_employee("Jameson");
    e.delete_row();
    commit_transaction();
}

// Scan multiple rows
TEST_F(gaia_object_test, new_set_ins) {
    begin_transaction();
    create_employee("Harold");
    create_employee("Jameson");
    EXPECT_EQ(2, count_rows());
    commit_transaction();
}

// Read back from new, unsaved object
TEST_F(gaia_object_test, net_set_get) {
    // Note no transaction needed to create & use writer.
    auto w = employee_writer();
    w.name_last = "Smith";
    EXPECT_STREQ(w.name_last.c_str(), "Smith");

    auto_transaction_t tx;
    auto eid = w.insert_row();
    EXPECT_STREQ(w.name_last.c_str(), "Smith");
    auto employee = employee_t::get(eid);
    EXPECT_STREQ(w.name_last.c_str(), employee.name_last());
}

// Read original value from an inserted object
TEST_F(gaia_object_test, read_original_from_copy) {
    begin_transaction();
    auto e = create_employee("Zachary");
    EXPECT_STREQ("Zachary", e.name_first());
    commit_transaction();
}

// Insert a row with no field values
TEST_F(gaia_object_test, new_insert_get) {
    begin_transaction();

    auto e = employee_t::get(employee_writer().insert_row());
    auto name = e.name_first();
    auto hire_date = e.hire_date();

    EXPECT_EQ(name, nullptr);
    EXPECT_EQ(0, hire_date);

    commit_transaction();
}

// Read values from a non-inserted writer
TEST_F(gaia_object_test, new_get) {
    begin_transaction();
    auto w = employee_writer();
    auto name = w.name_first;
    auto hire_date = w.hire_date;

    EXPECT_TRUE(name.empty());
    EXPECT_EQ(0, hire_date);
    commit_transaction();
}

// Attempt to insert with an update writer, this should work.
TEST_F(gaia_object_test, existing_insert_field) {
    begin_transaction();
    auto e = employee_t::get(employee_writer().insert_row());
    auto writer = e.writer();

    // This creates a new row.
    auto eid = writer.insert_row();
    EXPECT_NE(e.gaia_id(), employee_t::get(eid).gaia_id());
    commit_transaction();
}

// Test on existing objects found by ID
// ====================================

// Create, write two rows, read back by scan and verify
TEST_F(gaia_object_test, read_back_scan) {
    begin_transaction();
    auto eid = create_employee("Howard").gaia_id();
    auto eid2 = create_employee("Henry").gaia_id();
    commit_transaction();

    begin_transaction();
    auto e = employee_t::get_first();
    EXPECT_EQ(eid, e.gaia_id());
    EXPECT_STREQ("Howard", e.name_first());
    e = e.get_next();
    EXPECT_EQ(eid2, e.gaia_id());
    EXPECT_STREQ("Henry", e.name_first());
    commit_transaction();
}

// Used twice, below
void UpdateReadBack(bool update_flag) {
    auto_transaction_t tx;
    create_employee("Howard");
    create_employee("Henry");
    tx.commit();

    auto e = employee_t::get_first();
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
    tx.commit();

    e = employee_t::get_first();
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
TEST_F(gaia_object_test, new_delete_insert) {
    begin_transaction();
    auto e = create_employee("Hector");
    auto w = e.writer();
    e.delete_row();
    w.insert_row();
    commit_transaction();
}

// EXCEPTION conditions
// ====================

// Attempt to create a row outside of a transaction
TEST_F(gaia_object_test, no_tx) {
    EXPECT_THROW(create_employee("Harold"), transaction_not_open);
    // NOTE: the employee_t object is leaked here
}

// Scan beyond the end of the iterator.
TEST_F(gaia_object_test, scan_past_end) {
    auto_transaction_t tx;
    create_employee("Hvitserk");
    int count = 0;
    auto e = employee_t::list().begin();
    while (e != employee_t::list().end()) {
        count++;
        e++;
    }
    EXPECT_EQ(count, 1);
    e++;
    EXPECT_EQ(e == employee_t::list().end(), true);
    e++;
    EXPECT_EQ(e == employee_t::list().end(), true);
}

// Test pre/post increment of iterator.
TEST_F(gaia_object_test, pre_post_iterator) {
    auto_transaction_t tx;
    create_employee("Hvitserk");
    create_employee("Hubert");
    create_employee("Humphrey");
    auto e = employee_t::list().begin();
    EXPECT_STREQ((*e).name_first(), "Hvitserk");
    EXPECT_STREQ((*e++).name_first(), "Hvitserk");
    EXPECT_STREQ((*e).name_first(), "Hubert");
    EXPECT_STREQ((*++e).name_first(), "Humphrey");
    EXPECT_STREQ((*e).name_first(), "Humphrey");
    e++;
    EXPECT_EQ(e == employee_t::list().end(), true);
    ++e;
    EXPECT_EQ(e == employee_t::list().end(), true);
}

// Create row, try getting row from wrong type
TEST_F(gaia_object_test, read_wrong_type) {
    begin_transaction();
    auto e = create_employee("Howard");
    auto eid = e.gaia_id();
    commit_transaction();

    begin_transaction();
    try {
        auto a = address_t::get(eid);
    }
    catch (const exception& e) {
        string what = string(e.what());
        EXPECT_EQ(what.find("Requesting Gaia type address_t") != string::npos, true);
    }
    EXPECT_THROW(address_t::get(eid), edc_invalid_object_type);
    commit_transaction();
}

// Create, write two rows, read back by ID and verify
TEST_F(gaia_object_test, read_back_id) {
    auto_transaction_t tx;
    auto eid = create_employee("Howard").gaia_id();
    auto eid2 = create_employee("Henry").gaia_id();

    tx.commit();

    auto e = employee_t::get(eid);
    EXPECT_STREQ("Howard", e.name_first());
    e = employee_t::get(eid2);
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
}

TEST_F(gaia_object_test, new_del_field_ref) {
    // create GAIA-64 scenario
    begin_transaction();

    auto e = employee_t::get(employee_writer().insert_row());
    e.delete_row();
    // can't access data from a deleted row
    EXPECT_THROW(e.name_first(), invalid_node_id);

    // Can't get a writer from a deleted row either
    EXPECT_THROW(e.writer(), invalid_node_id);

    commit_transaction();
}

// Delete a found object then update
TEST_F(gaia_object_test, new_del_update) {
    begin_transaction();
    auto e = create_employee("Hector");
    e.delete_row();
    EXPECT_THROW(e.writer().update_row(), invalid_node_id);
    commit_transaction();
}

// Delete a found object then insert after, it's good again.
TEST_F(gaia_object_test, found_del_ins) {
    begin_transaction();

    auto e = create_employee("Hector");
    auto writer = e.writer();
    e.delete_row();
    EXPECT_THROW(e.writer(), invalid_node_id);
    // We got the writer before we deleted the row.
    // We can't update the row but we can insert a new one.
    auto eid = writer.insert_row();
    commit_transaction();

    begin_transaction();
    e = employee_t::get(eid);
    e.writer().name_first = "Hudson";
    commit_transaction();
}

// Delete a found object then update
TEST_F(gaia_object_test, found_del_update) {
    begin_transaction();
    auto e = create_employee("Hector");
    auto eid = e.gaia_id();
    commit_transaction();

    begin_transaction();
    e = employee_t::get(eid);
    auto w = e.writer();
    e.delete_row();
    EXPECT_THROW(w.update_row(), invalid_node_id);
    commit_transaction();
}

// The simplified model allows you to
// do multiple insertions with the same
// writer.  This seems reasonable given the model
// of passing in the row object to an insert_row method
// auto writer = Employe::writer();
// writer.insert_row();
// writer.insert_row();

// Simplified model doesn't allow this because you
// you cannot create a new employee_t() and the
// employee_t::writer method returns an employee_writer
// not an employee_t object.  The update and delete
// methods require a reference to an employee_t object
// and not a writer object.  The insert method
// only takes a writer object.

// Delete a row twice
TEST_F(gaia_object_test, new_del_del) {
    begin_transaction();
    auto e = create_employee("Hugo");
    // The first delete succeeds.
    e.delete_row();
    // The second one should throw.
    EXPECT_THROW(e.delete_row(), invalid_node_id);
    commit_transaction();
}

TEST_F(gaia_object_test, auto_tx_begin) {

    // Default constructor enables auto_begin semantics
    auto_transaction_t tx;

    auto writer = employee_writer();
    writer.name_last = "Hawkins";
    employee_t e = employee_t::get(writer.insert_row());
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
    auto writer = employee_writer();

    writer.name_last = "Hawkins";
    employee_t e = employee_t::get(writer.insert_row());
    tx.commit();

    // Expect an exception since we're not in a transaction
    EXPECT_THROW(e.name_last(), transaction_not_open);

    begin_transaction();

    EXPECT_STREQ(e.name_last(), "Hawkins");

    // This is legal.
    tx.commit();
}

TEST_F(gaia_object_test, auto_tx_rollback) {
    gaia_id_t id;
    {
        auto_transaction_t tx;
        auto writer = employee_writer();
        writer.name_last = "Hawkins";
        id = writer.insert_row();
    }
    // Transaction was rolled back
    auto_transaction_t tx;
    employee_t e = employee_t::get(id);
    EXPECT_FALSE(e);
}


TEST_F(gaia_object_test, writer_value_ref) {
    begin_transaction();
    employee_writer w1 = employee_writer();
    w1.name_last = "Gretzky";
    employee_t e = employee_t::get(w1.insert_row());
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
    {
        g_inserted_id = employee_t::insert_row(g_insert, nullptr, nullptr, 0, nullptr, nullptr);
    }
    commit_transaction();

    if (new_thread) { end_session(); }
}

const char* g_update = "update_thread";
void update_thread(gaia_id_t id)
{
    begin_session();
    begin_transaction();
    {
        employee_t e = employee_t::get(id);
        employee_writer w = e.writer();
        w.name_first = g_update;
        w.update_row();
    }
    commit_transaction();
    end_session();
}

void delete_thread(gaia_id_t id)
{
    begin_session();
    begin_transaction();
    {
        employee_t::delete_row(id);
    }
    commit_transaction();
    end_session();
}

TEST_F(gaia_object_test, thread_insert) {
    // Insert a record in another thread and verify
    // we can see it here.
    thread t = thread(insert_thread, true);
    t.join();
    begin_transaction();
    {
        employee_t e = employee_t::get(g_inserted_id);
        EXPECT_STREQ(e.name_first(), g_insert);
    }
    commit_transaction();
}

TEST_F(gaia_object_test, thread_update) {
    // Update a record in another thread and verify
    // we can see it here.
    insert_thread(false);

    begin_transaction();
    employee_t e = employee_t::get(g_inserted_id);
    EXPECT_STREQ(e.name_first(), g_insert);

    // Update the same record in a different transaction and commit.
    thread t = thread(update_thread, g_inserted_id);
    t.join();

    // The change should not be visible in our transaction
    EXPECT_STREQ(e.name_first(), g_insert);
    commit_transaction();

    begin_transaction();
    {
        // now we should have the new value
        EXPECT_STREQ(e.name_first(), g_update);
    }
    commit_transaction();
}

TEST_F(gaia_object_test, thread_update_conflict) {
    insert_thread(false);

    begin_transaction();
    {
        // Update the same record in a different transaction and commit
        thread t = thread(update_thread, g_inserted_id);
        t.join();

        employee_writer w = employee_t::get(g_inserted_id).writer();
        w.name_first = "Violation";
        w.update_row();

    }
    EXPECT_THROW(commit_transaction(), transaction_update_conflict);

    begin_transaction();
    {
        // Actual value here is g_update, which shows that my update never
        // went through.
        EXPECT_STREQ(employee_t::get(g_inserted_id).name_first(), g_update);
    }
    commit_transaction();
}

TEST_F(gaia_object_test, thread_update_other_row) {
    gaia_id_t row1_id = 0;
    gaia_id_t row2_id = 0;

    begin_transaction();
    {
        row1_id = employee_t::insert_row(g_insert, nullptr, nullptr, 0, nullptr, nullptr);
        row2_id = employee_t::insert_row(g_insert, nullptr, nullptr, 0, nullptr, nullptr);
    }
    commit_transaction();

    begin_transaction();
    {
        // Update the same record in a different transaction and commit
        thread t = thread(update_thread, row1_id);
        t.join();

        employee_writer w = employee_t::get(row2_id).writer();
        w.name_first = "No Violation";
        w.update_row();
    }
    EXPECT_NO_THROW(commit_transaction());

    begin_transaction();
    {
        // Row 1 should have been updated by the update thread.
        // Row 2 should have been updated by this thread.
        EXPECT_STREQ(employee_t::get(row1_id).name_first(), g_update);
        EXPECT_STREQ(employee_t::get(row2_id).name_first(), "No Violation");
    }
    commit_transaction();
}

TEST_F(gaia_object_test, thread_delete) {
    // update a record in another thread and verify
    // we can see it
    insert_thread(false);
    begin_transaction();
    {
        // Delete the record we just inserted
        thread t = thread(delete_thread, g_inserted_id);
        t.join();

        // The change should not be visible in our transaction and
        // we should be able to access the record just fine.
        EXPECT_STREQ(employee_t::get(g_inserted_id).name_first(), g_insert);
    }
    commit_transaction();

    begin_transaction();
    {
        // Now this should fail.
        EXPECT_THROW(employee_t::get(g_inserted_id).name_first(), invalid_node_id);
    }
    commit_transaction();
}

TEST_F(gaia_object_test, thread_insert_update_delete) {
    // Do three concurrent operations and make sure we see are isolated from them
    // and then do see them in a subsequent transaction.
    const char* local = "My Insert";
    gaia_id_t row1_id = 0;
    gaia_id_t row2_id = 0;

    begin_transaction();
    {
        row1_id = employee_t::insert_row(local, nullptr, nullptr, 0, nullptr, nullptr);
        row2_id = employee_t::insert_row("Red Shirt", nullptr, nullptr, 0, nullptr, nullptr);
    }
    commit_transaction();

    begin_transaction();
    {
        thread t1 = thread(delete_thread, row2_id);
        thread t2 = thread(insert_thread, true);
        thread t3 = thread(update_thread, row1_id);
        t1.join();
        t2.join();
        t3.join();
        EXPECT_STREQ(employee_t::get(row1_id).name_first(), local);
        EXPECT_STREQ(employee_t::get(row2_id).name_first(), "Red Shirt");
    }
    commit_transaction();

    begin_transaction();
    {
        // Deleted row2.
        EXPECT_THROW(employee_t::get(row2_id).name_first(), invalid_node_id);
        // Inserted a new row
        EXPECT_STREQ(employee_t::get(g_inserted_id).name_first(), g_insert);
        // Updated row1.
        EXPECT_STREQ(employee_t::get(row1_id).name_first(), g_update);
    }
    commit_transaction();
};

TEST_F(gaia_object_test, thread_delete_conflict) {
    // Have two threads delete the same row.
    insert_thread(false);
    begin_transaction();
    {
        employee_t::delete_row(g_inserted_id);
        thread t1 = thread(delete_thread, g_inserted_id);
        t1.join();
    }
    EXPECT_THROW(commit_transaction(), transaction_update_conflict);

    begin_transaction();
    {
        // Expect the row to be deleted so another attempt to delete should fail.
        EXPECT_THROW(employee_t::delete_row(g_inserted_id), invalid_node_id);
    }
    commit_transaction();
};

// Pass by reference.
void employee_func_ref(const employee_t& e, const char* first_name)
{
    begin_transaction();
    {
        if (first_name)
        {
            EXPECT_STREQ(e.name_first(), first_name);
        }
        else
        {
            EXPECT_THROW(e.name_first(), invalid_node_id);
        }
    }
    commit_transaction();
}

// Pass by value, ensures copy constructor does the right thing.
void employee_func_val(employee_t e, const char* first_name)
{
    begin_transaction();
    {
        if (first_name)
        {
            EXPECT_STREQ(e.name_first(), first_name);
        }
        else
        {
            EXPECT_THROW(e.name_first(), invalid_node_id);
        }

    }
    commit_transaction();
}

TEST_F(gaia_object_test, default_construction) {
    // Valid use case to create an unbacked object that
    // you can't do anything with.  However, now you can
    // set a variable to it later in the function, use it as
    // a member of a class, etc.
    employee_t e;
    address_t a;

    employee_func_ref(e, nullptr);
    employee_func_val(e, nullptr);

    begin_transaction();
    {
        EXPECT_THROW(e.name_first(), invalid_node_id);
        EXPECT_THROW(a.addressee_employee(), invalid_node_id);
        EXPECT_THROW(e.manages_employee(), invalid_node_id);
        EXPECT_THROW(e.writer(), invalid_node_id);
        EXPECT_THROW(e.delete_row(), invalid_node_id);

        for (auto a : e.addressee_address_list())
        {
            printf("%s\n", a.state());
        }
        e = create_employee("Windsor");
        EXPECT_STREQ(e.name_first(), "Windsor");
    }
    commit_transaction();

    employee_func_ref(e, "Windsor");
    employee_func_val(e, "Windsor");

    begin_transaction();
    {
        EXPECT_STREQ(e.name_first(), "Windsor");
    }
    commit_transaction();
}

// Testing the arrow dereference operator->() in gaia_iterator_t.
TEST_F(gaia_object_test, iter_arrow_deref) {
    const char* emp_name = "Phillip";
    auto_transaction_t tx;

    create_employee(emp_name);
    tx.commit();

    gaia_iterator_t<employee_t> emp_iter = employee_t::list().begin();
    EXPECT_STREQ(emp_iter->name_first(), emp_name);
}

// Structure for message queue
constexpr int c_message_text_length = 1;
typedef struct {
    long type;
    char text[c_message_text_length];
} message_buffer_t;

// Test parallel multi-process creation of objects.
TEST_F(gaia_object_test, multi_process_inserts) {
    message_buffer_t message;

    end_session();

    // Common parent/child setup for a message queue.
    key_t queue_key = ftok("/tmp", 2020);
    int message_id = msgget(queue_key, 0666 | IPC_CREAT);
    pid_t child_pid = fork();
    message.type = 1;

    if (child_pid) {
        // PARENT PROCESS.
        begin_session();

        // EXCHANGE 1: serialized transactions.
        //   Add two employees in the parent. Commit.
        //   Add two employees in the child. Commit.
        //   Read and verify all employees in the parent.

        // Parent process adds two employees.
        begin_transaction();
        create_employee("Howard").gaia_id();
        create_employee("Henry").gaia_id();
        commit_transaction();

        // The child will add two employees. Wait for it to complete.
        msgsnd(message_id, &message, c_message_text_length, 0);
        alarm(1);
        msgrcv(message_id, &message, c_message_text_length, 0, 0);
        alarm(0);

        // Scan through all resulting rows.
        // See if all objects exist.
        begin_transaction();
        auto employee = employee_t::get_first();
        EXPECT_STREQ(employee.name_first(), "Howard");
        employee = employee.get_next();
        EXPECT_STREQ(employee.name_first(), "Henry");
        employee = employee.get_next();
        EXPECT_STREQ(employee.name_first(), "Harold");
        employee = employee.get_next();
        EXPECT_STREQ(employee.name_first(), "Hank");
        commit_transaction();

        // EXCHANGE 2: concurrent transactions.
        //   Begin transaction in parent. Add one employee.
        //   Begin child transaction. Add one employee. Commit.
        //   Commit transaction in parent. Verify employees.

        begin_transaction();
        create_employee("Hugo").gaia_id();

        msgsnd(message_id, &message, c_message_text_length, 0);
        // If the child process fails, this will hang forever. Set up an alarm
        // so that it will break out after a second.
        alarm(1);
        // If msgrcv is interrupted by the alarm, it returns -1.
        EXPECT_GT(msgrcv(message_id, &message, c_message_text_length, 0, 0), 0);
        alarm(0);

        commit_transaction();

        // Scan through all resulting rows.
        // See if all objects exist.
        begin_transaction();
        employee = employee_t::get_first();
        EXPECT_STREQ(employee.name_first(), "Howard");
        employee = employee.get_next();
        EXPECT_STREQ(employee.name_first(), "Henry");
        employee = employee.get_next();
        EXPECT_STREQ(employee.name_first(), "Harold");
        employee = employee.get_next();
        EXPECT_STREQ(employee.name_first(), "Hank");
        employee = employee.get_next();
        EXPECT_STREQ(employee.name_first(), "Hugo");
        // employee = employee.get_next();
        // EXPECT_STREQ(employee.name_first(), "Hubert");
        commit_transaction();

        wait(&child_pid);
        // Clean up the message queue.
        msgctl(message_id, IPC_RMID, NULL);

    }
    else {
        // CHILD PROCESS.
        begin_session();

        // EXCHANGE 1: serialized transactions.

        // Wait for the "go".
        msgrcv(message_id, &message, c_message_text_length, 0, 0);

        begin_transaction();
        create_employee("Harold").gaia_id();
        create_employee("Hank").gaia_id();
        commit_transaction();

        // Tell parent to "go".
        msgsnd(message_id, &message, c_message_text_length, 0);

        // EXCHANGE 2: concurrent transactions.
        msgrcv(message_id, &message, c_message_text_length, 0, 0);

        begin_transaction();
        create_employee("Hubert").gaia_id();
        commit_transaction();

        msgsnd(message_id, &message, c_message_text_length, 0);

        // The parent process is waiting for this one to terminate.
        end_session();
        exit(0);
    }

}
