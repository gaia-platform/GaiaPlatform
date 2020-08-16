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
std::map<gaia_id_t, employee_t> deleted_employee_map;

void validate_data() {
    begin_session();
    uint64_t count = 0;
    begin_transaction();
    for (auto employee = employee_t::get_first(); employee; employee.get_next()) {
        auto it = employee_map.find(employee.gaia_id());
        // Validate that employee is found.
        assert(it != employee_map.end());

        auto employee_expected = it->second;

        // Validate employee fields.
        assert(employee_expected.email() == employee.email());
        assert(employee_expected.name_last() == employee.name_last());
        assert(employee_expected.name_first() == employee.name_first());
        assert(employee_expected.ssn() == employee.ssn());
        assert(employee_expected.hire_date() == employee.hire_date());
        assert(employee_expected.web() == employee.web());

        count ++;
    }

    cout << "Total count before recovery " << employee_map.size() << endl << flush;
    cout << "Total count after recovery " << count << endl << flush;
    assert(count == employee_map.size());
    commit_transaction();
    end_session();

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
gaia_id_t generate_employee_record() {
    employee_writer w;
    w.name_first = generate_string(128);
    w.name_last = generate_string(128);
    w.ssn = generate_string(128);
    w.hire_date = rand();
    w.email = generate_string(128);
    w.web = generate_string(128);

    gaia_id_t id = w.insert_row();
    return id;
}

void load_data(uint64_t total_size_bytes, bool kill_server_during_load, db_server_t& server, const char* path) {
    auto records = total_size_bytes / employee_record_size_bytes + 1;
    
    auto number_of_transactions = records / load_batch_size + 1;

    cout << "Loading data: Total number of records " << records << endl << flush;
    cout << "Loading data: Total number of transactions " << number_of_transactions << endl << flush;

    begin_session();
    // Load data in multiple transactions.
    for (uint64_t transaction_id = 1; transaction_id <= number_of_transactions; transaction_id++) {
        // Load a batch per transaction.
        std::map<gaia_id_t, employee_t> temp_employee_map;
        uint64_t idd = 0;
        begin_transaction();
        for (uint64_t batch_count = 1; batch_count <= load_batch_size; batch_count++) {
            // Insert row.
            auto id = generate_employee_record();
            auto e = employee_t::get(id);
            idd = id;
            temp_employee_map.insert(make_pair(id, e));
        }
        commit_transaction();

        begin_transaction();
        assert(employee_t::get(idd).gaia_id() == idd);
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

    end_session();
}

void delete_all() {
    gaia::db::begin_session();
    cout << "Deleting all records" << endl << flush;
    begin_transaction();
    int count = 0;
    for (auto employee = employee_t::get_first(); employee; employee.get_next()) {
        employee.delete_row();
        count ++;
    }
    commit_transaction();
    cout << "Deleted " << count << " records "<< endl << flush;
    gaia::db::end_session();
    validate_data();
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
    std::string server_dir_path = argv[1];
    employee_map.clear();
    restart_server(server, server_dir_path.data());
    delete_all();

    // 1) Load & Recover test - with data size less than write buffer size. 
    // All writes will be confined to the WAL & will not make it to SST (DB binary file)
    // Sigkill server.
    {
        // Start server.
        restart_server(server, server_dir_path.data());
        validate_data();
        // Load 1 MB data; write buffer size is 4MB.
        load_data(1 * 1024 * 1024, false, server, server_dir_path.data());
        validate_data();
        // Restart server & validate data.
        restart_server(server, server_dir_path.data());
        validate_data();
    }

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

    // 7) Delete everything and validate.
    delete_all();

    // End test.
    stop_server(server);
    return res;
}
