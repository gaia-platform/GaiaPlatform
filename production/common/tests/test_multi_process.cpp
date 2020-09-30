/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// NOTE to future generatons:
// This test spawns child processes to serve as a second user in the database.
// The child must be "clean" in the sense that it cannot be started with
// handles and threads from the parent process that require separate cleanup.
// For this reason, it purposely does not initialize the logger, which
// spawns a pool of threads that interfere with the child process as it tries
// to exit.
//
// A better solution may be to create a separate program than can be started
// by this test program.
//
// If this test starts hanging while it runs, it is probably another form of
// this same issue.
//
// Please delete this note after you fix this.

#include <iostream>
#include <cstdlib>
#include <thread>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

#include "gtest/gtest.h"
#include "gaia_addr_book.h"
#include "db_test_base.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::addr_book;


// Utility function that creates one named employee row provided the writer.
employee_t insert_employee(employee_writer& writer, const char* name_first)
{
    writer.name_first = name_first;
    return employee_t::get(writer.insert_row());
}

// Utility function that creates one address row provided the writer.
address_t insert_address(address_writer& writer, const char* street, const char* city)
{
    writer.street = street;
    writer.city = city;
    return address_t::get(writer.insert_row());
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

constexpr const char c_go_child[] = "go_child";
constexpr const char c_go_parent[] = "go_parent";

// The multi_process fixture overrides SetUp() and TeadDown() because
// it needs to control when begin_session() and end_session() are called.
class gaia_multi_process_test : public db_test_base_t {
protected:
    pid_t child_pid;
    sem_t* sem_go_child;
    sem_t* sem_go_parent;
    timespec timeout;
    void SetUp() override {
        reset_server(false);
        sem_unlink(c_go_child);
        sem_unlink(c_go_parent);
    }
    void TearDown() override {
    }
    // Check the exit() status from the child process. If this is called
    // after an error in the parent, it's because the child has probably
    // had an error. This function will detect which error it was.
    void check_child_pid(pid_t pid) {
        int status;
        waitpid(pid, &status, 0);
        // Did the child exit()?
        ASSERT_EQ(WIFEXITED(status), true);
        // If so, did it exit witout an error?
        ASSERT_EQ(WEXITSTATUS(status), 0);
    }
    // Preparation/creation of semaphores, used by parent process.
    void semaphore_startup() {
        // NOTE: even on slower CPUs, 4 seconds should be adequate for these tests.
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 4;

        // Common parent/child setup for semaphore synchronization.
        // Semaphore must be opened before second process starts because it assumes
        // that the semaphore exists.
        sem_go_child = sem_open(c_go_child, O_CREAT, S_IWUSR | S_IRUSR, 0);
        ASSERT_NE(sem_go_child, SEM_FAILED) << "failed to open sem_go_child";
        sem_go_parent = sem_open(c_go_parent, O_CREAT, S_IRUSR | S_IWUSR, 0);
        ASSERT_NE(sem_go_parent, SEM_FAILED) << "failed to open sem_go_parent";
    }
    // Open existing semaphores, used by child process.
    void semaphore_open() {
        sem_go_child = sem_open(c_go_child, 0);
        sem_go_parent = sem_open(c_go_parent, 0);
    }
    // Close & remove semaphore names, used by parent process.
    void semaphore_cleanup() {
        sem_close(sem_go_child);
        sem_close(sem_go_parent);
        sem_unlink(c_go_child);
        sem_unlink(c_go_parent);
    }
};

// Test parallel multi-process transactions.
TEST_F(gaia_multi_process_test, multi_process_inserts) {

    semaphore_startup();

    pid_t child_pid = fork();

    if (child_pid) {
        // PARENT PROCESS.
        begin_session();

        // EXCHANGE 1: serialized transactions.
        //   Add two employees in the parent. Commit.
        //   Add two employees in the child. Commit.
        //   Read and verify all employees in the parent.

        // Parent process adds two employees.
        begin_transaction();
        create_employee("Howard");
        create_employee("Henry");
        commit_transaction();

        // The child will add two employees. Wait for it to complete.
        sem_post(sem_go_child);
        if (sem_timedwait(sem_go_parent, &timeout) == -1) {
            ASSERT_EQ(errno, ETIMEDOUT);
            end_session();
            check_child_pid(child_pid);
        }

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
        create_employee("Hugo");

        sem_post(sem_go_child);
        if (sem_timedwait(sem_go_parent, &timeout) == -1) {
            ASSERT_EQ(errno, ETIMEDOUT);
            end_session();
            check_child_pid(child_pid);
        }

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
        employee = employee.get_next();
        EXPECT_STREQ(employee.name_first(), "Hubert");
        commit_transaction();

        end_session();

        check_child_pid(child_pid);

        // Clean up the semaphores.
        semaphore_cleanup();

    }
    else {
        // CHILD PROCESS.

        // Open pre-existing semaphores.
        semaphore_open();

        begin_session();

        // EXCHANGE 1: serialized transactions.

        // Wait for the "go".
        if (sem_timedwait(sem_go_child, &timeout) == -1) {
            exit(1);
        }

        try {
            begin_transaction();
            create_employee("Harold");
            create_employee("Hank");
            commit_transaction();
        }
        catch (gaia_exception& e) {
            cerr << "Error in child process: " << e.what() << endl;
            exit(2);
        }

        // Tell parent to "go".
        sem_post(sem_go_parent);

        // EXCHANGE 2: concurrent transactions.
        if (sem_timedwait(sem_go_child, &timeout) == -1) {
            exit(3);
        }

        try {
            begin_transaction();
            create_employee("Hubert");
            commit_transaction();
        }
        catch (gaia_exception& e) {
            cerr << "Error in child process: " << e.what() << endl;
            exit(4);
        }

        sem_post(sem_go_parent);

        // The parent process is waiting for this one to terminate.
        end_session();
        exit(0);
    }

}

// Test parallel multi-process transactions and aborts.
TEST_F(gaia_multi_process_test, multi_process_aborts) {
    semaphore_startup();

    pid_t child_pid = fork();

    if (child_pid) {
        // PARENT PROCESS.
        begin_session();

        // EXCHANGE 1: serialized transactions.
        //   Add two employees in the parent. Commit.
        //   Add an employee in the child. Abort.
        //   Add another employee in the child. Commit.
        //   Read and verify all employees in the parent.

        // Parent process adds two employees.
        begin_transaction();
        create_employee("Howard");
        create_employee("Henry");
        commit_transaction();

        // The child will add two employees, rolling back one of them. Wait for it to complete.
        sem_post(sem_go_child);
        if (sem_timedwait(sem_go_parent, &timeout) != 0) {
            end_session();
            check_child_pid(child_pid);
        }

        // Scan through all resulting rows.
        // See if all objects exist.
        begin_transaction();
        auto empl_iterator = employee_t::list().begin();
        ++empl_iterator;
        empl_iterator++;
        // Make sure we have hit the end of the list.
        EXPECT_STREQ((*empl_iterator).name_first(), "Hank");
        empl_iterator++;
        EXPECT_EQ(true, empl_iterator == employee_t::list().end());
        commit_transaction();

        // EXCHANGE 2: concurrent transactions.
        //   Begin transaction in parent. Add one employee.
        //   Begin child transaction. Add one employee. Commit.
        //   Abort transaction in parent. Verify employees.

        begin_transaction();
        create_employee("Hugo");

        sem_post(sem_go_child);
        if (sem_timedwait(sem_go_parent, &timeout) == -1) {
            ASSERT_EQ(errno, ETIMEDOUT);
            end_session();
            check_child_pid(child_pid);
        }

        rollback_transaction();

        // Scan through all resulting rows.
        // See if all objects exist.
        begin_transaction();
        auto employee = employee_t::get_first();
        employee = employee.get_next();
        employee = employee.get_next();
        employee = employee.get_next();
        EXPECT_STREQ(employee.name_first(), "Hubert");
        commit_transaction();

        end_session();

        check_child_pid(child_pid);

        // Clean up the semaphores.
        semaphore_cleanup();

    }
    else {
        // CHILD PROCESS.

        // Open pre-existing semaphores.
        semaphore_open();

        begin_session();

        // EXCHANGE 1: serialized transactions.

        // Wait for the "go".
        if (sem_timedwait(sem_go_child, &timeout) == -1) {
            exit(1);
        }

        try {
            begin_transaction();
            create_employee("Harold");
            rollback_transaction();
            begin_transaction();
            create_employee("Hank");
            commit_transaction();
        }
        catch (gaia_exception& e) {
            cerr << "Error in child process: " << e.what() << endl;
            exit(2);
        }

        // Tell parent to "go".
        sem_post(sem_go_parent);

        // EXCHANGE 2: concurrent transactions.
        if (sem_timedwait(sem_go_child, &timeout) == -1) {
            exit(3);
        }

        try {
            begin_transaction();
            create_employee("Hubert");
            commit_transaction();
        }
        catch (gaia_exception& e) {
            cerr << "Error in child process: " << e.what() << endl;
            exit(4);
        }

        sem_post(sem_go_parent);

        // The parent process is waiting for this one to terminate.
        end_session();
        exit(0);
    }
}

// Create objects in one process, connect them in another, verify in first process.
TEST_F(gaia_multi_process_test, multi_process_conflict) {
    semaphore_startup();

    if ((child_pid = fork())) {
        // PARENT PROCESS.
        begin_session();

        begin_transaction();
        employee_writer employee_w;
        auto e1 = insert_employee(employee_w, "Horace");
        commit_transaction();
        begin_transaction();
        address_writer address_w;
        auto a2 = insert_address(address_w, "10618 129th Pl. N.E.", "Kirkland");
        auto a3 = insert_address(address_w, "10805 Circle Dr.", "Bothell");
        e1.addressee_address_list().insert(a2);
        e1.addressee_address_list().insert(a3);

        // Let the child process run and complete during this transaction.
        sem_post(sem_go_child);
        if (sem_timedwait(sem_go_parent, &timeout) == -1) {
            ASSERT_EQ(errno, ETIMEDOUT);
            end_session();
            check_child_pid(child_pid);
        }

        EXPECT_THROW(commit_transaction(), transaction_update_conflict);

        check_child_pid(child_pid);

        // Count the members. Only one succeeded.
        begin_transaction();
        int count = 0;
        for (auto a : e1.addressee_address_list()) {
            count++;
        }
        commit_transaction();
        EXPECT_EQ(count, 1);

        end_session();

        sem_close(sem_go_child);
        sem_close(sem_go_parent);
        sem_unlink(c_go_child);
        sem_unlink(c_go_parent);
    }
    else {
        // CHILD PROCESS.

        // Open pre-existing semaphores.
        semaphore_open();

        begin_session();

        // Wait for the "go".
        if (sem_timedwait(sem_go_child, &timeout) == -1) {
            end_session();
            exit(1);
        }

        try {
            begin_transaction();
            // Locate the employee object.
            auto e1 = employee_t::get_first();
            address_writer address_w;
            auto a1 = insert_address(address_w, "430 S. 41st St.", "Boulder");
            e1.addressee_address_list().insert(a1);
            commit_transaction();
        }
        catch (gaia_exception& e) {
            cerr << "Error in child process: " << e.what() << endl;
            exit(2);
        }

        sem_post(sem_go_parent);

        // The parent process is waiting for this one to terminate.
        end_session();
        exit(0);
    }
}

// Create objects in one process, connect them in another, verify in first process.
TEST_F(gaia_multi_process_test, multi_process_commit) {
    semaphore_startup();

    if ((child_pid = fork())) {
        // PARENT PROCESS.
        begin_session();

        begin_transaction();
        employee_writer employee_w;
        auto e1 = insert_employee(employee_w, "Horace");
        commit_transaction();
        begin_transaction();
        address_writer address_w;
        auto a2 = insert_address(address_w, "10618 129th Pl. N.E.", "Kirkland");
        auto a3 = insert_address(address_w, "10805 Circle Dr.", "Bothell");
        e1.addressee_address_list().insert(a2);
        e1.addressee_address_list().insert(a3);
        commit_transaction();

        // Let the child process run and complete during this transaction.
        sem_post(sem_go_child);
        if (sem_timedwait(sem_go_parent, &timeout) == -1) {
            ASSERT_EQ(errno, ETIMEDOUT);
            end_session();
            check_child_pid(child_pid);
        }

        check_child_pid(child_pid);

        // Count the members. All should have succeeded.
        begin_transaction();
        int count = 0;
        for (auto a : e1.addressee_address_list()) {
            count++;
        }
        commit_transaction();
        EXPECT_EQ(count, 3);

        end_session();

        sem_close(sem_go_child);
        sem_close(sem_go_parent);
        sem_unlink(c_go_child);
        sem_unlink(c_go_parent);
    }
    else {
        // CHILD PROCESS.

        // Open pre-existing semaphores.
        semaphore_open();

        begin_session();

        // Wait for the "go".
        if (sem_timedwait(sem_go_child, &timeout) == -1) {
            exit(1);
        }

        try {
            begin_transaction();
            // Locate the employee object.
            auto e1 = employee_t::get_first();
            address_writer address_w;
            auto a1 = insert_address(address_w, "430 S. 41st St.", "Boulder");
            e1.addressee_address_list().insert(a1);
            commit_transaction();
        }
        catch (gaia_exception& e) {
            cerr << "Error in child process: " << e.what() << endl;
            exit(2);
        }

        sem_post(sem_go_parent);

        // The parent process is waiting for this one to terminate.
        end_session();
        exit(0);
    }
}
