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
// Don't cache direct access objects as they will
// point to garbage values post crash recovery.
struct employee_copy_t {
    string name_first; 
    string name_last;
    string ssn;
    int64_t hire_date;
    string email;
    string web; 
};

// Map of employees for which the server has returned a successful commit.
// We maintain this map in memory & will use it to validate recovered shared memory post crash.
std::map<gaia_id_t, employee_copy_t> employee_map;

void validate_data() {
    uint64_t count = 0;
    begin_transaction();
    // These are pointers, so if the shared memory gets screwed, the object gets screwed.
    // Which is why expected employee is null, and it points to gaia_table instead.
    // Deserialization is broken.
    // So it should be employee map copy.
    for (auto employee = employee_t::get_first(); employee; employee = employee.get_next()) {
    // while (employee != employee_t::list().end()) {
        auto it = employee_map.find(employee.gaia_id());
        // Validate that emempployee is found.
        assert(it != employee_map.end());

        auto employee_expected = it->second;

        // Validate employee fields.
        // cout << "[emp name] is: " << employee_expected.name_first() << ":" << employee.name_first() << endl << flush;

        assert(strcmp(employee_expected.email.data(), employee.email()) == 0); 
        assert(strcmp(employee_expected.name_last.data(), employee.name_last()) == 0);
        assert(strcmp(employee_expected.name_first.data(), employee.name_first()) == 0);
        assert(strcmp(employee_expected.ssn.data(), employee.ssn()) == 0);
        assert(employee_expected.hire_date == employee.hire_date());
        assert(strcmp(employee_expected.web.data(), employee.web()) == 0);

        count++;
    }

    cout << "Total count before recovery " << employee_map.size() << endl << flush;
    cout << "Total count after recovery " << count << endl << flush;
    assert(count == employee_map.size());
    commit_transaction();
    cout << "Validation complete." << endl << flush;
}

void restart_server(db_server_t& server, const char* path) {
    server.start(path);
}

void stop_server(db_server_t& server) {
    server.stop();
}

// Random updates & deletes.
// Perform multiple transactions.
void modify_data() {
    for (int i = 0; i < employee_map.size() / 3; i++) {
        auto it = employee_map.find(rand() % employee_map.size());
        employee_t e = employee_t::get(it->first);
        
    }
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
        std::map<gaia_id_t, employee_copy_t> temp_employee_map;
        begin_transaction();
        for (uint64_t batch_count = 1; batch_count <= load_batch_size; batch_count++) {
            // Insert row.
            auto e = generate_employee_record();
            temp_employee_map.insert(make_pair(e.gaia_id(), employee_copy_t{e.name_first(), e.name_last(), e.ssn(), e.hire_date(), e.email(), e.web()}));
        }
        commit_transaction();

        employee_map.insert(temp_employee_map.begin(), temp_employee_map.end());
        temp_employee_map.clear();
        assert(temp_employee_map.size() == 0);

        // Crash during load.
        if (kill_server_during_load && transaction_id % 5 == 0) {
            cout << "Crash during load" << endl << flush;
            end_session();
            restart_server(server, path);
            begin_session();
            validate_data();
        }

        if (transaction_id % 25 == 0) {
            cout << "Loading data: Executed " << transaction_id << " transactions ..." << endl << flush;
        }
    }

    cout << "Load completed for " << employee_map.size() << " records."<< endl << flush;
}

int get_count() {
    int total_count = 0;
    begin_transaction();
    for (auto employee = employee_t::get_first(); employee; employee = employee.get_next()) {
        total_count++;
    }
    commit_transaction();
    return total_count;
}

void delete_all() {
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
        employee_map.erase(id);
    }

    cout << "Deleted " << count << " records "<< endl << flush;
    validate_data();
    assert(get_count() == 0);
}

void load_recover_test(db_server_t server, std::string server_dir_path, uint64_t load_size_bytes, int crash_validate_loop_count, bool kill_during_workload) {
    // Start server.
    restart_server(server, server_dir_path.data());
    begin_session();
    delete_all();
    // load_data(load_size_bytes, kill_during_workload, server, server_dir_path.data());
    begin_transaction();
    cout << "Loading "<< endl << flush;
    auto e1 = generate_employee_record();
    auto e2 = generate_employee_record();
    //     string name_first; 
    // string name_last;
    // string ssn;
    // int64_t hire_date;
    // string email;
    // string web; 
    employee_map.insert(make_pair(e1.gaia_id(), employee_copy_t{e1.name_first(), e1.name_last(), e1.ssn(), e1.hire_date(), e1.email(), e1.web()}));
    employee_map.insert(make_pair(e2.gaia_id(), employee_copy_t{e2.name_first(), e2.name_last(), e2.ssn(), e2.hire_date(), e2.email(), e2.web()}));
    commit_transaction();
    validate_data();
    end_session();

    // Restart server & validate data.
    for (int i = 0; i < crash_validate_loop_count; i++) {
        restart_server(server, server_dir_path.data());
        begin_session();
        cout << "Count post recovery: " << get_count() << endl << flush;
        validate_data();
        end_session();
    }

    restart_server(server, server_dir_path.data());
    begin_session();
    delete_all();
    end_session();

    // Validate all data deleted.
    restart_server(server, server_dir_path.data());
    begin_session();
    assert(get_count() == 0);
    end_session();
    stop_server(server);
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

    // 1) Load & Recover test - with data size less than write buffer size; 
    // All writes will be confined to the WAL & will not make it to SST (DB binary file)
    // Sigkill server.
    {
        load_recover_test(server, server_dir_path, 0.1 * 1024 * 1024, 2, true); 
    }

    // 2) Load (more data) & Recover test - with data size greater than write buffer size. 
    // Writes will exist in both the WAL & SST files.
    {
        // load_recover_test(server, server_dir_path, 5 * 1024 * 1024); 
    }

    // Todo (msj)
    // Validate gaia_id is not recycled post Recovery.

    return res;
}
