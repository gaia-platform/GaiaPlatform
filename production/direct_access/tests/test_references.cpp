/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <fstream>
#include "gtest/gtest.h"
#include "gaia_addr_book.h"
#include "db_test_base.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::addr_book;

class gaia_references_test : public db_test_base_t {
protected:
    void SetUp() override {
        db_test_base_t::SetUp();
    }

    void TearDown() override {
        db_test_base_t::TearDown();
    }
};


// Test connecting, disconnecting, navigating records
// ==================================================
TEST_F(gaia_references_test, connect) {
    begin_transaction();

    // Connect two inserted rows.
    employee_writer ew;
    ew.name_first = "Hidalgo";
    employee_t e3 = employee_t::get(ew.insert_row());

    address_writer aw;
    aw.city = "Houston";
    address_t a3 = address_t::get(aw.insert_row());

    e3.addresses_list().insert(a3);
    int count = 0;
    for (auto ap : e3.addresses_list()) {
        if (ap) {
            count++;
        }
    }
    EXPECT_EQ(count, 1 );

    e3.addresses_list().erase(a3);
    a3.delete_row();
    e3.delete_row();
    commit_transaction();
}


employee_t create_hierarchy() {
    auto eptr = employee_t::get(
        employee_t::insert_row("Heidi", "Humphry", "555-22-4444", 20200530, "heidi@gmail.com", "")
    );
    for (int i = 0; i<200; i++) {
        char addr_string[6];
        sprintf(addr_string, "%d", i);
        auto aptr = address_t::get(
            address_t::insert_row(addr_string, addr_string, addr_string, addr_string, addr_string, addr_string, true)
        );
        eptr.addresses_list().insert(aptr);
        for (int j = 0; j < 20; j++) {
            char phone_string[5];
            sprintf(phone_string, "%d", j);
            auto pptr = phone_t::get(
                    phone_t::insert_row(phone_string, phone_string, true)
            );
            aptr.phones_list().insert(pptr);
        }
    }
    return eptr;
}

int scan_hierarchy(employee_t& eptr) {
    int count = 1;
    for (auto aptr : eptr.addresses_list()) {
        ++count;
        for (auto pptr : aptr.phones_list()) {
            if (pptr) {
                ++count;
            }
        }
    }
    return count;
}

bool bounce_hierarchy(employee_t& eptr) {
    // Take a subset of the hierarchy and travel to the bottom. From the bottom, travel back
    // up, verifying the results on the way.
    int count_addresses = 0;
    for (auto aptr : eptr.addresses_list()) {
        if ((++count_addresses % 30) == 0) {
            int count_phones = 0;
            for (auto pptr : aptr.phones_list()) {
                if ((++count_phones % 4) == 0) {
                    auto up_aptr = pptr.phones();
                    EXPECT_EQ(up_aptr, aptr);
                    auto up_eptr = up_aptr.addresses();
                    EXPECT_EQ(up_eptr, eptr);
                }
            }
        }
    }
    return true;
}

bool delete_hierarchy(employee_t& eptr) {
    int count_addresses = 1;
    while (count_addresses>=1) {
        count_addresses = 0;
        // As long as there is at least one address_t, continue
        address_t* xaptr;
        for (auto aptr : eptr.addresses_list()) {
            ++count_addresses;
            xaptr = &aptr;
            // Repeat: delete the last phone until all are deleted
            int count_phones = 1;
            while (count_phones>=1) {
                count_phones = 0;
                phone_t* xpptr;
                for (auto pptr : aptr.phones_list()) {
                    ++count_phones;
                    xpptr = &pptr;
                }
                if (count_phones) {
                    aptr.phones_list().erase(*xpptr);
                    xpptr->delete_row();
                }
            }
        }
        if (count_addresses) {
            eptr.addresses_list().erase(*xaptr);
            xaptr->delete_row();
        }
    }
    eptr.delete_row();
    return true;
}

template <typename T_type>
int count_type() {
    int count = 0;
    for (auto row : T_type::list()) {
        row.gaia_type();
        count++;
    }
    return count;
}

string first_employee() {
    const char* name;
    for (auto row : employee_t::list()) {
        name = row.name_first();
    }
    return name;
}

int all_addresses() {
    int count = 0;
    char addr_string[6];
    for (auto address : address_t::list()) {
        sprintf(addr_string, "%d", count);
        EXPECT_STREQ(addr_string, address.city());
        count++;
    }
    int i = 0;
    for (auto it = address_t::list().begin(); it != address_t::list().end(); ++it) {
        sprintf(addr_string, "%d", i);
        EXPECT_STREQ(addr_string, (*it).city());
        count--;
        ++i;
    }

    return count;
}

// Create a hierachy of records, then scan and count them.
TEST_F(gaia_references_test, connect_scan) {
    begin_transaction();

    // Create a hierarchy of employee to address to phone
    auto eptr = create_hierarchy();

    // Removing a row involved in any set should be prevented.
    EXPECT_THROW(eptr.delete_row(), node_not_disconnected);

    // Count the records in the hierarchy
    auto record_count = scan_hierarchy(eptr);
    EXPECT_EQ(record_count, 4201);

    // Travel down, then up the hierarchy
    EXPECT_EQ(bounce_hierarchy(eptr), true);

    // Count the rows.
    EXPECT_EQ(count_type<employee_t>(), 1);
    EXPECT_EQ(count_type<address_t>(), 200);
    EXPECT_EQ(count_type<phone_t>(), 4000);

    // Scan through some rows.
    EXPECT_EQ(first_employee(), "Heidi");

    // Scan through all addresses.
    EXPECT_EQ(all_addresses(), 0);

    // Delete the hierarchy, every third record, until it's gone
    EXPECT_EQ(delete_hierarchy(eptr), true);
    commit_transaction();
}

void scan_manages(vector<string>& employee_vector, employee_t& e) {
    employee_vector.push_back(e.name_first());
    for (auto eptr : e.manages_list()) {
        scan_manages(employee_vector, eptr);
    }
}

employee_t insert_employee(employee_writer& writer, const char* name_first)
{
    writer.name_first = name_first;
    return employee_t::get(writer.insert_row());
}

address_t insert_address(address_writer& writer, const char* street, const char* city)
{
    writer.street = street;
    writer.city = city;
    return address_t::get(writer.insert_row());
}

// Test recursive scanning, employee_t to employee_t through manages relationship.
TEST_F(gaia_references_test, recursive_scan) {
    begin_transaction();

    // The "manages" set is employee_t to employee_t.
    // This test will create, then walk through a management hierarchy.
    // Horace
    //    Henry
    //       Hal
    //       Hiram
    //          Howard
    //    Hector
    //    Hank

    employee_writer writer;
    auto e1 = insert_employee(writer, "Horace");
    auto e2 = insert_employee(writer, "Henry");
    auto e3 = insert_employee(writer, "Hal");
    auto e4 = insert_employee(writer, "Hiram");
    auto e5 = insert_employee(writer, "Howard");
    auto e6 = insert_employee(writer, "Hector");
    auto e7 = insert_employee(writer, "Hank");

    e1.manages_list().insert(e2); // Horace to Henry
    e2.manages_list().insert(e3); //    Henry to Hal
    e2.manages_list().insert(e4); //    Henry to Hiram
    e4.manages_list().insert(e5); //       Hiram to Howard
    e1.manages_list().insert(e6); // Horace to Hector
    e1.manages_list().insert(e7); // Horace to Hank

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

// Re-hydrate IDs created in prior transaction, then connect.
TEST_F(gaia_references_test, connect_to_ids) {
    auto_transaction_t tx;

    /* Create some unconnected Employee and Address objects */
    employee_writer employee_w;
    employee_w.name_first = "Horace";
    gaia_id_t eid1 = employee_w.insert_row();

    address_writer address_w;
    address_w.street = "430 S. 41st St.";
    address_w.city = "Boulder";
    gaia_id_t aid1 = address_w.insert_row();

    address_w.street = "10618 129th Pl. N.E.";
    address_w.city = "Kirkland";
    gaia_id_t aid2 = address_w.insert_row();

    // Start a new transaction so that the objects are created fresh.
    tx.commit();

    // Generate the object from the ids.
    employee_t e1 = employee_t::get(eid1);
    address_t a1 = address_t::get(aid1);
    address_t a2 = address_t::get(aid2);
    e1.addresses_list().insert(a1);
    e1.addresses_list().insert(a2);
}

// Connect objects created in prior transaction.
TEST_F(gaia_references_test, connect_after_tx) {
    auto_transaction_t tx;

    employee_writer employee_w;
    auto e1 = insert_employee(employee_w, "Horace");

    address_writer address_w;
    auto a1 = insert_address(address_w, "430 S. 41st St.", "Boulder");
    auto a2 = insert_address(address_w, "10618 129th Pl. N.E.", "Kirkland");

    // In a subsequent transaction, connect the objects.
    tx.commit();

    // Use the objects from the previous transaction.
    e1.addresses_list().insert(a1);
    e1.addresses_list().insert(a2);
    auto addr = e1.addresses_list().begin();
    EXPECT_STREQ((*addr).city(), "Kirkland");
    ++addr;
    EXPECT_STREQ((*addr).city(), "Boulder");
}

// TEST_F(gaia_references_test, writer_update) {
//     auto_transaction_t tx;

//     employee_writer employee_w;
//     auto e1 = insert_employee(employee_w, "Horace");

//     tx.commit();
//     employee_writer empl_w = e1.writer();
//     EXPECT_STREQ(empl_w.name_first.c_str(), "Horace");
//     empl_w.name_first = "Hubert";
//     EXPECT_STREQ(e1.name_first(), "Horace");
//     EXPECT_STREQ(empl_w.name_first.c_str(), "Hubert");
//     empl_w.update_row();
//     EXPECT_STREQ(empl_w.name_first.c_str(), "Hubert");
//     EXPECT_STREQ(e1.name_first(), "Hubert");
// }

// Erase list members inserted in prior transaction.
TEST_F(gaia_references_test, disconnect_after_tx) {
    auto_transaction_t tx;

    employee_writer employee_w;
    auto e1 = insert_employee(employee_w, "Horace");

    address_writer address_w;
    auto a1 = insert_address(address_w, "430 S. 41st St.", "Boulder");
    auto a2 = insert_address(address_w, "10618 129th Pl. N.E.", "Kirkland");

    e1.addresses_list().insert(a1);
    e1.addresses_list().insert(a2);

    // In a subsequent transaction, disconnect the objects.
    tx.commit();

    e1.addresses_list().erase(a1);
    e1.addresses_list().erase(a2);
}

// Generate an exception by attempting to insert member twice.
TEST_F(gaia_references_test, connect_twice) {
    auto_transaction_t tx;

    /* Create some unconnected Employee and Address objects */
    employee_writer employee_w;
    auto e1 = insert_employee(employee_w, "Horace");

    address_writer address_w;
    auto a1 = insert_address(address_w, "430 S. 41st St.", "Boulder");

    // The second insert should fail.
    e1.addresses_list().insert(a1);
    EXPECT_THROW(e1.addresses_list().insert(a1), edc_already_inserted);
}

// Generate an exception by attempting to erase un-inserted member.
TEST_F(gaia_references_test, erase_uninserted) {
    auto_transaction_t tx;

    employee_writer employee_w;
    auto e1 = insert_employee(employee_w, "Horace");

    address_writer address_w;
    auto a1 = insert_address(address_w, "430 S. 41st St.", "Boulder");

    // The erase() should fail.
    EXPECT_THROW(e1.addresses_list().erase(a1), edc_invalid_member);

    // Now insert it, erase, and erase again.
    e1.addresses_list().insert(a1);
    e1.addresses_list().erase(a1);
    EXPECT_THROW(e1.addresses_list().erase(a1), edc_invalid_member);
}

// Make sure that erasing a member found in iterator doesn't crash.
TEST_F(gaia_references_test, erase_in_iterator) {
    auto_transaction_t tx;

    employee_writer employee_w;
    auto e1 = insert_employee(employee_w, "Horace");

    address_writer address_w;
    auto a1 = insert_address(address_w, "430 S. 41st St.", "Boulder");
    auto a2 = insert_address(address_w, "10618 129th Pl. N.E.", "Kirkland");
    auto a3 = insert_address(address_w, "10805 Circle Dr.", "Bothell");

    e1.addresses_list().insert(a1);
    e1.addresses_list().insert(a2);
    e1.addresses_list().insert(a3);

    // We're happy with it not crashing, even though the list is cut short.
    int count = 0;
    for (auto a : e1.addresses_list()) {
        e1.addresses_list().erase(a);
        count++;
    }
    EXPECT_EQ(count,1);

    // There should be two on the list here, but same behavior.
    count = 0;
    for (auto a : e1.addresses_list()) {
        e1.addresses_list().erase(a);
        count++;
    }
    EXPECT_EQ(count,1);

    // Verify that one member remains.
    count = 0;
    for (auto a : e1.addresses_list()) {
        EXPECT_STREQ(a.city(), "Boulder");
        count++;
    }
    EXPECT_EQ(count,1);
}

// Scan beyond the end of the iterator.
TEST_F(gaia_references_test, scan_past_end) {
    auto_transaction_t tx;

    employee_writer employee_w;
    auto e1 = insert_employee(employee_w, "Horace");

    address_writer address_w;
    auto a1 = insert_address(address_w, "430 S. 41st St.", "Boulder");
    auto a2 = insert_address(address_w, "10618 129th Pl. N.E.", "Kirkland");
    auto a3 = insert_address(address_w, "10805 Circle Dr.", "Bothell");

    e1.addresses_list().insert(a1);
    e1.addresses_list().insert(a2);
    e1.addresses_list().insert(a3);

    int count = 0;
    auto a = e1.addresses_list().begin();
    while (a != e1.addresses_list().end()) {
        count++;
        a++;
    }
    EXPECT_EQ(count,3);
    a++;
    EXPECT_EQ(a == e1.addresses_list().end(), true);
    a++;
    EXPECT_EQ(a == e1.addresses_list().end(), true);
}

// Attempt to insert two EDC objects in separate thread.
void insert_object(bool committed, employee_t e1, address_t a1)
{
    begin_session();
    begin_transaction();
    {
        if (committed) {
            e1.addresses_list().insert(a1);
        }
        else {
            // Nothing is committed yet.
            EXPECT_THROW(e1.addresses_list().insert(a1), edc_invalid_state);
        }
    }
    commit_transaction();
    end_session();
}

// Attempt to insert objects hydrated from IDs, in separate thread.
void insert_addresses(bool committed, gaia_id_t eid1, gaia_id_t aid1, gaia_id_t aid2, gaia_id_t aid3)
{
    begin_session();
    begin_transaction();
    {
        auto e1 = employee_t::get(eid1);
        auto a1 = address_t::get(aid1);
        auto a2 = address_t::get(aid2);
        auto a3 = address_t::get(aid3);
        if (committed) {
            EXPECT_THROW(e1.addresses_list().insert(a1), edc_already_inserted);
            e1.addresses_list().insert(a2);
            e1.addresses_list().insert(a3);
        }
        else {
            EXPECT_THROW(e1.addresses_list().insert(a1), edc_invalid_state);
        }
    }
    commit_transaction();
    end_session();
}

// Create objects in one thread, connect them in another, verify in first thread.
TEST_F(gaia_references_test, thread_inserts) {
    auto_transaction_t tx;

    employee_writer employee_w;
    auto e1 = insert_employee(employee_w, "Horace");

    address_writer address_w;
    auto a1 = insert_address(address_w, "430 S. 41st St.", "Boulder");
    auto a2 = insert_address(address_w, "10618 129th Pl. N.E.", "Kirkland");
    auto a3 = insert_address(address_w, "10805 Circle Dr.", "Bothell");

    // These threads should have problems since the objects aren't committed yet.
    thread t = thread(insert_object, false, e1, a1);
    t.join();

    t = thread(insert_addresses, false, e1.gaia_id(), a1.gaia_id(), a2.gaia_id(), a3.gaia_id());
    t.join();

    tx.commit();

    // Retry the threads after our objects are committed.
    t = thread(insert_object, true, e1, a1);
    t.join();
    t = thread(insert_addresses, true, e1.gaia_id(), a1.gaia_id(), a2.gaia_id(), a3.gaia_id());
    t.join();

    // Count the members. They should show up.
    int count = 0;
    for (auto a : e1.addresses_list()) {
        count++;
    }
    EXPECT_EQ(count, 3);
}
