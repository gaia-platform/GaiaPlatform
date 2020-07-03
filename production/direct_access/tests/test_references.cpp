/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include "gtest/gtest.h"
#include "addr_book_gaia_mock.h"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace AddrBook;

class gaia_references_test : public ::testing::Test {
protected:
    void delete_employees() {
        begin_transaction();
        for(Employee_ptr e = Employee::get_first();
            e;
            e = Employee::get_first())
        {
            try {
                e->delete_row();
            }
            catch (gaia_exception&) {
                // Connection tests may cause this, but that's okay.
                break;
            }
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


// Test connecting, disconnecting, navigating records
// ==================================================
TEST_F(gaia_references_test, connect) {
    begin_transaction();

    // In simplified API this is not possible since
    // you cannot get a non-inserted Employee or Address
    // object.
    
    // Connect two new rows.
    //Employee_writer ew = Employee::create();
    //Address_writer aw = Address::create();
    //Employee_ptr e1 = Employee::get_row_by_id(Employee::insert_row(ew));
    //Address_ptr a1 = Address::get_row_by_id(Address::insert_row(aw));
    //EXPECT_THROW(e1->address_list.insert(a1), edc_unstored_row);

    // In simplified API this is not possible since
    // you cannot get a non-inserted Employee or Address
    // object.

    // Connect two non-inserted rows.
    //Employee* e2 = new Employee();
    //e2->set_name_first("Howard");
    //Address* a2 = new Address();
    //a2->set_city("Houston");
    //EXPECT_THROW(e2->address_list.insert(a2), edc_unstored_row);

    // Connect two inserted rows.
    Employee_writer ew = Employee::create();
    ew->name_first = "Hidalgo";
    Employee_ptr e3 = Employee::get_row_by_id(Employee::insert_row(ew));

    Address_writer aw = Address::create();
    aw->city = "Houston";
    Address_ptr a3 = Address::get_row_by_id(Address::insert_row(aw));

    e3->address_list.insert(a3);
    int count = 0;
    for (auto ap : e3->address_list) {
        if (ap) {
            count++;
        }
    }
    EXPECT_EQ(count, 1);
    commit_transaction();
}


Employee_ptr create_hierarchy() {
    auto eptr = Employee::get_row_by_id(
        Employee::insert_row("Heidi", "Humphry", "555-22-4444", 20200530, "heidi@gmail.com", "")
    );
    for (int i = 0; i<200; i++) {
        char addr_string[6];
        sprintf(addr_string, "%d", i);
        auto aptr = Address::get_row_by_id(
            Address::insert_row(addr_string, addr_string, addr_string, addr_string, addr_string, addr_string, true)
        );
        eptr->address_list.insert(aptr);
        for (int j = 0; j < 20; j++) {
            char phone_string[5];
            sprintf(phone_string, "%d", j);
            auto pptr = Phone::get_row_by_id(
                    Phone::insert_row(phone_string, phone_string, true)
            );
            aptr->phone_list.insert(pptr);
        }
    }
    return eptr;
}

int scan_hierarchy(Employee_ptr eptr) {
    int count = 1;
    for (auto aptr : eptr->address_list) {
        ++count;
        for (auto pptr : aptr->phone_list) {
            if (pptr) {
                ++count;
            }
        }
    }
    return count;
}

bool bounce_hierarchy(Employee_ptr eptr) {
    // Take a subset of the hierarchy and travel to the bottom. From the bottom, travel back
    // up, verifying the results on the way.
    int count_addresses = 0;
    for (auto aptr : eptr->address_list) {
        if ((++count_addresses % 30) == 0) {
            int count_phones = 0;
            for (auto pptr : aptr->phone_list) {
                if ((++count_phones % 4) == 0) {
                    auto up_aptr = pptr->address_owner();
                    EXPECT_EQ(up_aptr, aptr);
                    auto up_eptr = up_aptr->employee_owner();
                    EXPECT_EQ(up_eptr, eptr);
                }
            }
        }
    }
    return true;
}

bool delete_hierarchy(Employee_ptr eptr) {
    int count_addresses = 1;
    while (count_addresses>=1) {
        count_addresses = 0;
        // As long as there is at least one Address, continue
        Address_ptr xaptr;
        for (auto aptr : eptr->address_list) {
            ++count_addresses;
            xaptr = aptr;
            // Repeat: delete the last phone until all are deleted
            int count_phones = 1;
            while (count_phones>=1) {
                count_phones = 0;
                Phone_ptr xpptr;
                for (auto pptr : aptr->phone_list) {
                    ++count_phones;
                    xpptr = pptr;
                }
                if (count_phones) {
                    aptr->phone_list.erase(xpptr);
                    xpptr->delete_row();
                }
            }
        }
        if (count_addresses) {
            eptr->address_list.erase(xaptr);
            xaptr->delete_row();
        }
    }
    eptr->delete_row();
    return true;
}

TEST_F(gaia_references_test, connect_scan) {
    begin_transaction();

    // Create a hierarchy of employee to address to phone
    auto eptr = create_hierarchy();

    // Removing a row involved in any set should be prevented.
    EXPECT_THROW(eptr->delete_row(), node_not_disconnected);

    // Count the records in the hierarchy
    auto record_count = scan_hierarchy(eptr);
    EXPECT_EQ(record_count, 4201);

    // Travel down, then up the hierarchy
    EXPECT_EQ(bounce_hierarchy(eptr), true);

    // Delete the hierarchy, every third record, until it's gone
    EXPECT_EQ(delete_hierarchy(eptr), true);
    commit_transaction();
}

void scan_manages(vector<string>& employee_vector, Employee_ptr e) {
    employee_vector.push_back(e->name_first());
    for (auto eptr : e->manages_employee_list) {
        scan_manages(employee_vector, eptr);
    }
}

Employee_ptr insert_employee(Employee_writer& writer, const char * name_first)
{
    writer->name_first = name_first;
    return Employee::get_row_by_id(Employee::insert_row(writer));
}

TEST_F(gaia_references_test, recursive_scan) {
    begin_transaction();

    // The "manages" set is Employee to Employee.
    // This test will create, then walk through a management hierarchy.
    // Horace
    //    Henry
    //       Hal
    //       Hiram
    //          Howard
    //    Hector
    //    Hank

    Employee_writer writer = Employee::create();
    auto e1 = insert_employee(writer, "Horace");
    auto e2 = insert_employee(writer, "Henry");
    auto e3 = insert_employee(writer, "Hal");
    auto e4 = insert_employee(writer, "Hiram");
    auto e5 = insert_employee(writer, "Howard");
    auto e6 = insert_employee(writer, "Hector");
    auto e7 = insert_employee(writer, "Hank");

    e1->manages_employee_list.insert(e2); // Horace to Henry
    e2->manages_employee_list.insert(e3); //    Henry to Hal
    e2->manages_employee_list.insert(e4); //    Henry to Hiram
    e4->manages_employee_list.insert(e5); //       Hiram to Howard
    e1->manages_employee_list.insert(e6); // Horace to Hector
    e1->manages_employee_list.insert(e7); // Horace to Hank

    // Recursive walk through hierarchy
    vector<string> employee_vector;
    scan_manages(employee_vector, e1);
    for (auto it = employee_vector.begin(); it != employee_vector.end(); it++) {
        if ((*it) != "Horace" &&
            (*it) != "Henry"  &&
            (*it) != "Hal"    &&
            (*it) != "Hiram"  &&
            (*it) != "Howard" &&
            (*it) != "Hector" &&
            (*it) != "Hank")
        {
            EXPECT_STREQ((*it).c_str(), "") << "Name was not found in hierarchy";
        }
    }

    commit_transaction();
}
