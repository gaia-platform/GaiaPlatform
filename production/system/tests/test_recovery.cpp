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
#include "gaia_addr_book.h"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::addr_book;

// Write 16 records in a single transaction.
size_t load_batch_size = 16;
// Size of a single record.
size_t employee_record_size_bytes = 648;
// Size of a string field in a record.
size_t field_size_bytes = 128;

size_t total_validation_loop_count = 10;

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
    size_t count = 0;
    begin_transaction();
    for (auto employee = employee_t::get_first(); employee; employee = employee.get_next()) {
    // while (employee != employee_t::list().end()) {
        auto it = employee_map.find(employee.gaia_id());
        
        if (it == employee_map.end()) {
            // There might be other tests that create employees. 
            continue;
        }

        auto employee_expected = it->second;

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

gaia_id_t get_random_map_key(std::map<gaia_id_t, employee_copy_t> m) {  
    auto it = m.begin();
    std::advance(it, rand() % m.size());
    return it->first;
}

void restart_server(db_server_t& server, const char* path) {
    server.start(path);
}

void stop_server(db_server_t& server) {
    server.stop();
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

// Random updates & deletes.
void modify_data() {
    std::set<gaia_id_t> to_delete_set;
    for (size_t i = 0; i < employee_map.size() / 2; i++) {
        begin_transaction();
        auto to_update = employee_map.find(get_random_map_key(employee_map));
        employee_t e1 = employee_t::get(to_update->first);
        auto w1 = e1.writer();
        auto name_first = generate_string(field_size_bytes);
        w1.name_first = name_first;
        w1.update_row();
        commit_transaction();
        to_update->second.name_first = name_first;

        auto to_delete = employee_map.find(get_random_map_key(employee_map));
        to_delete_set.insert(to_delete->first);
    }

    cout << "[Modify record] Number of records to delete: " << to_delete_set.size() << endl << flush;

    for (gaia_id_t id : to_delete_set) {
        employee_map.erase(id);
        begin_transaction();
        auto e = employee_t::get(id);
        e.delete_row();
        commit_transaction();
    }
}

// Method will generate an employee record of size 128 * 5 + 8 (648) bytes.
employee_t generate_employee_record() {
    auto w = employee_writer();
    w.name_first = generate_string(field_size_bytes);
    w.name_last = generate_string(field_size_bytes);
    w.ssn = generate_string(field_size_bytes);
    w.hire_date = rand();
    w.email = generate_string(field_size_bytes);
    w.web = generate_string(field_size_bytes);

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

void delete_all(int initial_record_count) {
    cout << "Deleting all records" << endl << flush;
    begin_transaction();
    int total_count = 0;

    // Cache entries to delete.
    std::set<gaia_id_t> to_delete;
    for (auto employee = employee_t::get_first(); employee; employee = employee.get_next()) {
        total_count++;
        to_delete.insert(employee.gaia_id());
    }
    cout << "To delete " << total_count << " records "<< endl << flush;
    commit_transaction();

    int count = 0;
    
    while (true) {
        begin_transaction();
        auto to_delete_copy = to_delete;
        for (gaia_id_t id : to_delete_copy) {
            auto e = employee_t::get(id);
            try {
                e.delete_row();
            } catch (const node_not_disconnected& e) {
                continue;
            }
            count ++;
            employee_map.erase(id);
            to_delete.erase(id);    
        }
        commit_transaction();
        cout << "Remaining count " << get_count() << endl << flush;
        if(get_count() == 0 || get_count() == initial_record_count) {
            break;
        }
    }

    cout << "Deleted " << count << " records "<< endl << flush;
    validate_data();
}

void load_modify_recover_test(db_server_t server,
    std::string server_dir_path,
    uint64_t load_size_bytes,
    int crash_validate_loop_count,
    bool kill_during_workload) {
    int initial_record_count; 
    // Start server.
    restart_server(server, server_dir_path.data());
    begin_session();
    initial_record_count = get_count();
    delete_all(initial_record_count);
    load_data(load_size_bytes, kill_during_workload, server, server_dir_path.data());
    validate_data();
    end_session();

    // Restart server, modify & validate data.
    for (int i = 0; i < crash_validate_loop_count; i++) {
        restart_server(server, server_dir_path.data());
        begin_session();
        cout << "Count post recovery: " << get_count() << endl << flush;
        validate_data();
        cout << "Modify begin: " << endl << flush;
        modify_data();
        cout << "Modify complete: " << endl << flush;
        validate_data();
        end_session();
    }

    restart_server(server, server_dir_path.data());
    begin_session();
    delete_all(initial_record_count);
    end_session();

    // Validate all data deleted.
    restart_server(server, server_dir_path.data());
    begin_session();
    assert(get_count() == initial_record_count);
    end_session();
    stop_server(server);
}

// Bad FD
void throw_after_restart_no_cleanup(db_server_t server, std::string server_dir_path) {
    restart_server(server, server_dir_path.data());
    begin_session();
    begin_transaction();
    generate_employee_record();
    restart_server(server, server_dir_path.data());
    // Try insert & then commit. The insert will succeed but the commit will fail.
    generate_employee_record();
    try {
        commit_transaction();
    } catch (no_session_active&) {
        cout << "Success " << endl << flush;
    } catch (exception&) {
        assert(false);
    }
}

// Bad FD
void throw_after_restart_clean_shutdown(db_server_t server, std::string server_dir_path) {
    restart_server(server, server_dir_path.data());
    begin_session();
    begin_transaction();
    generate_employee_record();
    commit_transaction();
    end_session();

    restart_server(server, server_dir_path.data());
    // Try begin.
    begin_transaction();
    end_session();
}

/**
 * Test Recovery with a single threaded client.
 * 
 * Sample usage:
 * test_recovery "/home/ubuntu/GaiaPlatform/production/build/db/storage_engine"
 * 
 * Todo(msj) - split up this test so that some simple recovery tests run with ctest and 
 * a longer test runs on teamcity.
 */
int main(int, char *argv[]) {
    int result = 0;
    db_server_t server;
    // Path of directory where server executable resides.
    std::string server_dir_path = argv[1];
    employee_map.clear();

    // 1) Server throws exception if restart occurs during client transaction.
    {
        throw_after_restart_clean_shutdown(server, server_dir_path);
    }

    // 2) Load & Recover test - with data size less than write buffer size; 
    // All writes will be confined to the WAL & will not make it to SST (DB binary file)
    // Sigkill server.
    {
        // load_modify_recover_test(server, server_dir_path, 0.1 * 1024 * 1024, 2, true); 
    }

    // 3) Load (more data) & Recover test - with data size greater than write buffer size. 
    // Writes will exist in both the WAL & SST files.
    // Test is switched off as it takes some time to run. Ideally, recovery test should be 
    // run on teamcity.
    {
        // load_modify_recover_test(server, server_dir_path, 16 * 1024 * 1024, 1, false); 
    }

    // Todo (msj)
    // 4) Validate gaia_id is not recycled post crash.

    return result;
}
