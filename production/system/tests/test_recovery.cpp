/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "gaia_system.hpp"
#include "gaia_db.hpp"
#include "db_test_base.hpp"
#include "gaia_addr_book_db.h"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::addr_book_db;

// Write 16 records in a single transaction.
size_t load_batch_size = 16;
// Size of a single record.
size_t employee_record_size_bytes = 648;

// Map of employees for which the server has returned a successful commit.
// We maintain this map in memory & will use it to validate recovered shared memory post crash.
std::map<gaia_id_t, employee_t> employee_map;

void validate_data() {
    // begin_session();
    uint64_t count = 0;
    begin_transaction();
    auto employee = employee_t::list().begin();
    while (employee != employee_t::list().end()) {
        auto it = employee_map.find((*employee).gaia_id());
        // Validate that employee is found.
        assert(it != employee_map.end());

        auto employee_expected = it->second;

        // Validate employee fields.
        assert(employee_expected.email() == (*employee).email());
        assert(employee_expected.name_last() == (*employee).name_last());
        assert(employee_expected.name_first() == (*employee).name_first());
        assert(employee_expected.ssn() == (*employee).ssn());
        assert(employee_expected.hire_date() == (*employee).hire_date());
        assert(employee_expected.web() == (*employee).web());

        employee++;
        count++;

        // cout << "Validated record with ID: " << (*employee).gaia_id() << endl << flush;
    }

    cout << "Total count before recovery " << employee_map.size() << endl << flush;
    cout << "Total count after recovery " << count << endl << flush;
    assert(count == employee_map.size());
    commit_transaction();

    cout << "Validation complete." << endl << flush;
    // end_session();

}

// Validate that ordering when updating/deleting the same keys is maintained post recovery.
bool validate_ordering() {
    return false;
}

void restart_server(db_server_t& server, const char* path, bool sigterm = false) {
    if (sigterm) {
        server.sigterm_stop();
        server.start(path, false);
    } else {
        server.start(path);
    }
}

void stop_server(db_server_t& server) {
    server.stop();
}

// Random updates & deletes.
// Perform multiple transactions.
void modify_data() {
    
}

std::string generate_string( size_t length_in_bytes )
{
    auto randchar = []() -> char
    {
        const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    std::string str(length_in_bytes,0);
    std::generate_n(str.begin(), length_in_bytes, randchar);
    return str;
}

// Method will generate an employee record of size 128 * 5 + 8 (648) bytes.
employee_t generate_employee_record() {
    auto w = employee_writer();
    w.name_first = generate_string(128);
    w.name_last = generate_string(128);
    w.ssn = generate_string(128);
    w.hire_date = rand();
    w.email = generate_string(128);
    w.web = generate_string(128);

    gaia_id_t id = w.insert_row();
    auto e = employee_t::get(id);

    assert(e.name_first() == w.name_first);
    return e;
}

void load_data(uint64_t total_size_bytes, bool kill_server_during_load, db_server_t& server, const char* path) {
    auto records = total_size_bytes / employee_record_size_bytes + 1;
    
    auto number_of_transactions = records / load_batch_size + 1;

    cout << "Loading data: Total number of records " << records << endl << flush;
    cout << "Loading data: Total number of transactions " << number_of_transactions << endl << flush;

    // Load data in multiple transactions.
    for (uint64_t transaction_id = 1; transaction_id <= number_of_transactions; transaction_id++) {
        // Load a batch per transaction.
        std::map<gaia_id_t, employee_t> temp_employee_map;
        uint64_t idd = 0;
        begin_transaction();
        for (uint64_t batch_count = 1; batch_count <= load_batch_size; batch_count++) {
            // Insert row.
            auto e = generate_employee_record();
            temp_employee_map.insert(make_pair(e.gaia_id(), e));
        }
        commit_transaction();

        begin_transaction();
        int c = 0;
        for (auto employee = employee_t::get_first(); employee; employee = employee.get_next()) {
            c++;
        }
        cout << "Count of employees " << c << endl << flush;
        commit_transaction();

        employee_map.insert(temp_employee_map.begin(), temp_employee_map.end());
        temp_employee_map.clear();
        assert(temp_employee_map.size() == 0);

        // Repeatedly crash after each transaction.
        if (kill_server_during_load) {
            restart_server(server, path);
            validate_data();
        }

        if (transaction_id % 10 == 0) {
            cout << "Loading data: Executed " << transaction_id << " transactions ..." << endl << flush;
        }
    }

    cout << "Load completed." << endl << flush;
}

void delete_all() {
    // gaia::db::begin_session();
    cout << "Deleting all records" << endl << flush;
    begin_transaction();
    int total_count = 0;

    // Cache entries to delete.
    std::vector<gaia_id_t> to_delete;
    for (auto employee = employee_t::get_first(); employee; employee = employee.get_next()) {
        total_count++;
        to_delete.push_back(employee.gaia_id());
    }
    cout << "To delete " << total_count << " records "<< endl << flush;
    commit_transaction();

    int count = 0;
    
    for (gaia_id_t id : to_delete) {
        begin_transaction();
        auto e = employee_t::get(id);
        e.delete_row();
        count ++;
        commit_transaction();
        cout << "Delete completed for commit ID " << id << endl << flush;

    }

    cout << "Deleted " << count << " records "<< endl << flush;
    // gaia::db::end_session();
    // validate_data();
}

/**
 * Test Recovery with a single threaded client.
 * 
 * Sample usage:
 * test_recovery "/home/ubuntu/GaiaPlatform/production/build/db/storage_engine"
 */
int main(int, char *argv[]) {
    int res = 0;
    db_server_t server;
    // Path of directory where server executable resides.
    std::string server_dir_path = "/home/ubuntu/GaiaPlatform/production/build/db/storage_engine";//argv[1];
    employee_map.clear();
    // restart_server(server, server_dir_path.data());

    for (int i = 1; i <= 1; i++) {
        cout << "Loop number " << i << endl << flush; // start server
        // Just write a single record.
        restart_server(server, server_dir_path.data());
        begin_session();
        begin_transaction();
        generate_employee_record();
        generate_employee_record();
        commit_transaction();
        end_session();

        int j = 0; // server start

        restart_server(server, server_dir_path.data());
        begin_session();
        // begin_transaction();
        // validate_data();
        delete_all();
        // commit_transaction();
        end_session();
        stop_server(server);
    }
    // begin_session();
    // delete_all();

    // 1) Load & Recover test - with data size less than write buffer size. 
    // All writes will be confined to the WAL & will not make it to SST (DB binary file)
    // Sigkill server.
    // {
    //     // Start server.
    //     // end_session();
    //     // restart_server(server, server_dir_path.data());
    //     // begin_session();
    //     // validate_data();
    //     // Load 1 MB data; write buffer size is 4MB.
    //     load_data(1 * 1024 * 1024, false, server, server_dir_path.data());
    //     validate_data();
    //     // Restart server & validate data.
    //     end_session();

    //     restart_server(server, server_dir_path.data());
    //     begin_session();
    //     validate_data();
    //     end_session();

    //     begin_session();
    //     // delete_all();
    // }

    // 2) Load & Recover test - with data size less than write buffer size. 
    // All writes will be confined to the WAL & will never make it to SST (DB binary file)
    // Sigterm server.

    // 3) Load (more data) & Recover test - with data size greater than write buffer size. 
    // Writes will exist in both the WAL & SST files.
    {
        // Start server.
        // restart_server(server, server_dir_path.data());

        // // Load 16MB data; this will force a flush of the RocksDB write buffer to multiple (4) SST files.
        // load_data(16 * 1024 * 1024, false);

        // // Restart server & validate data.
        // restart_server(server, server_dir_path.data());
        // validate_data();
    }

    // 4) Crash and recover multiple times.

    // 5) Randomly modify (Update & Delete) entries & Recover. 
    // Validate that ordering of events (upon Recovery) is maintained.

    // 6) CRUD operations with multiple random restarts.

    // 7) Delete everything and validate gaia_id is not zero!

    // End test.
    // end_session();
    // stop_server(server);
    return res;
}
