/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

#include "db_catalog_test_base.hpp"
#include "gaia_addr_book.h"
#include "gaia_relationships.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::addr_book;

class gaia_references_test_t : public db_catalog_test_base_t
{
protected:
    gaia_references_test_t()
        : db_catalog_test_base_t(std::string("addr_book.ddl")){};

    static gaia_id_t find_invalid_id()
    {
        // Starting from an high ID to reduce the chances of collision
        // with already existent ids.
        const int c_lower_id_range = 10000;

        // 1M oughta to be enough.
        const int c_higher_id_range = 10000 * 100;

        for (int i = c_lower_id_range; i < c_higher_id_range; i++)
        {
            auto invalid_obj = gaia_ptr::open(i);

            if (!invalid_obj)
            {
                return i;
            }
        }

        throw runtime_error(
            "Impossible to find an invalid ID in the range "
            + to_string(c_lower_id_range) + " - " + to_string(c_higher_id_range));
    }
};

// Test connecting, disconnecting, navigating records
// ==================================================
TEST_F(gaia_references_test_t, connect)
{
    begin_transaction();

    // Connect two inserted rows.
    employee_writer ew;
    ew.name_first = "Hidalgo";
    employee_t e3 = employee_t::get(ew.insert_row());

    address_writer aw;
    aw.city = "Houston";
    address_t a3 = address_t::get(aw.insert_row());

    e3.addressee_address_list().insert(a3);
    int count = 0;
    for (auto const& ap : e3.addressee_address_list())
    {
        if (ap)
        {
            count++;
        }
    }
    EXPECT_EQ(count, 1);

    e3.addressee_address_list().erase(a3);
    a3.delete_row();
    e3.delete_row();
    commit_transaction();
}

// Repeat above test, but with gaia_id_t members only.
TEST_F(gaia_references_test_t, connect_id_member)
{
    begin_transaction();

    // Connect two inserted rows.
    employee_writer ew;
    ew.name_first = "Hidalgo";
    employee_t e3 = employee_t::get(ew.insert_row());

    address_writer aw;
    aw.city = "Houston";
    gaia_id_t aid3 = aw.insert_row();

    e3.addressee_address_list().insert(aid3);
    int count = 0;
    for (auto const& ap : e3.addressee_address_list())
    {
        if (ap)
        {
            count++;
        }
    }
    EXPECT_EQ(count, 1);

    gaia_id_t invalid_id = find_invalid_id();

    e3.addressee_address_list().erase(aid3);
    address_t::delete_row(aid3);
    e3.delete_row();
    EXPECT_THROW(address_t::delete_row(invalid_id), invalid_node_id);
    commit_transaction();
}

employee_t create_hierarchy()
{
    const int hire_date = 20200530;
    const int count_addresses = 200;
    const int count_phones = 20;
    const int addr_size = 6;
    const int phone_size = 5;
    auto employee
        = employee_t::get(employee_t::insert_row("Heidi", "Humphry", "555-22-4444", hire_date, "heidi@gmail.com", ""));
    for (int i = 0; i < count_addresses; i++)
    {
        char addr_string[addr_size];
        sprintf(addr_string, "%d", i);
        auto address = address_t::get(
            address_t::insert_row(addr_string, addr_string, addr_string, addr_string, addr_string, addr_string, true));
        employee.addressee_address_list().insert(address);
        for (int j = 0; j < count_phones; j++)
        {
            char phone_string[phone_size];
            sprintf(phone_string, "%d", j);
            auto phone = phone_t::get(phone_t::insert_row(phone_string, phone_string, true));
            address.phone_list().insert(phone);
        }
    }
    return employee;
}

int scan_hierarchy(employee_t& eptr)
{
    int count = 1;
    for (auto aptr : eptr.addressee_address_list())
    {
        ++count;
        for (auto const& pptr : aptr.phone_list())
        {
            if (pptr)
            {
                ++count;
            }
        }
    }
    return count;
}

bool bounce_hierarchy(employee_t& eptr)
{
    // Take a subset of the hierarchy and travel to the bottom. From the bottom, travel back
    // up, verifying the results on the way.
    const int c_count_subset = 30;
    int count_addressee = 0;
    for (auto aptr : eptr.addressee_address_list())
    {
        if ((++count_addressee % c_count_subset) == 0)
        {
            int count_phones = 0;
            for (auto pptr : aptr.phone_list())
            {
                if ((++count_phones % 4) == 0)
                {
                    auto up_aptr = pptr.address();
                    EXPECT_EQ(up_aptr, aptr);
                    auto up_eptr = up_aptr.addressee_employee();
                    EXPECT_EQ(up_eptr, eptr);
                }
            }
        }
    }
    return true;
}

bool delete_hierarchy(employee_t& employee_to_delete)
{
    int count_addressee = 1;
    while (count_addressee >= 1)
    {
        count_addressee = 0;
        // As long as there is at least one address_t, continue
        address_t address_to_delete;
        for (auto& address : employee_to_delete.addressee_address_list())
        {
            ++count_addressee;
            address_to_delete = address;
            // Repeat: delete the last phone until all are deleted
            int count_phones = 1;
            while (count_phones >= 1)
            {
                count_phones = 0;

                phone_t phone_to_delete;
                for (const auto& phone : address.phone_list())
                {
                    ++count_phones;
                    phone_to_delete = phone;
                }

                if (count_phones)
                {
                    address.phone_list().erase(phone_to_delete);
                    phone_to_delete.delete_row();
                }
            }
        }
        if (count_addressee)
        {
            employee_to_delete.addressee_address_list().erase(address_to_delete);
            address_to_delete.delete_row();
        }
    }

    employee_to_delete.delete_row();
    return true;
}

template <typename T_type>
int count_type()
{
    int count = 0;
    for (auto row : T_type::list())
    {
        row.gaia_type();
        count++;
    }
    return count;
}

string first_employee()
{
    const char* name = nullptr;
    for (auto const& row : employee_t::list())
    {
        name = row.name_first();
    }
    return name;
}

int all_addressee()
{
    int count = 0;
    const int addr_size = 6;
    char addr_string[addr_size];
    for (auto const& address : address_t::list())
    {
        sprintf(addr_string, "%d", count);
        EXPECT_STREQ(addr_string, address.city());
        count++;
    }
    int i = 0;
    for (auto it = address_t::list().begin(); it != address_t::list().end(); ++it)
    {
        sprintf(addr_string, "%d", i);
        EXPECT_STREQ(addr_string, (*it).city());
        count--;
        ++i;
    }

    return count;
}

// Create a hierachy of records, then scan and count them.
TEST_F(gaia_references_test_t, connect_scan)
{
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
    EXPECT_EQ(all_addressee(), 0);

    // Delete the hierarchy, every third record, until it's gone
    EXPECT_EQ(delete_hierarchy(eptr), true);
    commit_transaction();
}

void scan_manages(vector<string>& employee_vector, employee_t& e)
{
    employee_vector.emplace_back(e.name_first());
    for (auto eptr : e.manages_employee_list())
    {
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
TEST_F(gaia_references_test_t, recursive_scan)
{
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

    e1.manages_employee_list().insert(e2); // Horace to Henry
    e2.manages_employee_list().insert(e3); //    Henry to Hal
    e2.manages_employee_list().insert(e4); //    Henry to Hiram
    e4.manages_employee_list().insert(e5); //       Hiram to Howard
    e1.manages_employee_list().insert(e6); // Horace to Hector
    e1.manages_employee_list().insert(e7); // Horace to Hank

    // Recursive walk through hierarchy
    vector<string> employee_vector;
    scan_manages(employee_vector, e1);
    for (auto const& it : employee_vector)
    {
        if (it != "Horace" && it != "Henry" && it != "Hal" && it != "Hiram" && it != "Howard" && it != "Hector"
            && it != "Hank")
        {
            EXPECT_STREQ(it.c_str(), "") << "Name was not found in hierarchy";
        }
    }

    commit_transaction();
}

// Re-hydrate IDs created in prior transaction, then connect.
TEST_F(gaia_references_test_t, connect_to_ids)
{
    auto_transaction_t txn;

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

    txn.commit();

    // Generate the object from the ids.
    employee_t e1 = employee_t::get(eid1);
    address_t a1 = address_t::get(aid1);
    address_t a2 = address_t::get(aid2);
    e1.addressee_address_list().insert(a1);
    e1.addressee_address_list().insert(a2);
}

// Connect objects created in prior transaction.
TEST_F(gaia_references_test_t, connect_after_txn)
{
    auto_transaction_t txn;

    employee_writer employee_w;
    auto e1 = insert_employee(employee_w, "Horace");

    address_writer address_w;
    auto a1 = insert_address(address_w, "430 S. 41st St.", "Boulder");
    auto a2 = insert_address(address_w, "10618 129th Pl. N.E.", "Kirkland");

    // In a subsequent transaction, connect the objects.
    txn.commit();

    // Use the objects from the previous transaction.
    e1.addressee_address_list().insert(a1);
    e1.addressee_address_list().insert(a2);
    auto addr = e1.addressee_address_list().begin();
    EXPECT_STREQ((*addr).city(), "Kirkland");
    ++addr;
    EXPECT_STREQ((*addr).city(), "Boulder");
}

// Erase list members inserted in prior transaction.
TEST_F(gaia_references_test_t, disconnect_after_txn)
{
    auto_transaction_t txn;

    employee_writer employee_w;
    auto e1 = insert_employee(employee_w, "Horace");

    address_writer address_w;
    auto a1 = insert_address(address_w, "430 S. 41st St.", "Boulder");
    auto a2 = insert_address(address_w, "10618 129th Pl. N.E.", "Kirkland");

    e1.addressee_address_list().insert(a1);
    e1.addressee_address_list().insert(a2);

    // In a subsequent transaction, disconnect the objects.
    txn.commit();

    e1.addressee_address_list().erase(a1);
    e1.addressee_address_list().erase(a2);
}

// Generate an exception by attempting to insert member twice.
TEST_F(gaia_references_test_t, connect_twice)
{
    auto_transaction_t txn;

    /* Create some unconnected Employee and Address objects */
    employee_writer employee_w;
    auto e1 = insert_employee(employee_w, "Horace");
    auto e2 = insert_employee(employee_w, "Hector");

    address_writer address_w;
    auto a1 = insert_address(address_w, "430 S. 41st St.", "Boulder");

    // The second insert is redundant and is treated as a no-op.
    e1.addressee_address_list().insert(a1);
    e1.addressee_address_list().insert(a1);

    // The third insert is illegal because the address cannot be on the
    // same list of two owners.
    EXPECT_THROW(e2.addressee_address_list().insert(a1), child_already_referenced);
}

// Generate an exception by attempting to erase un-inserted member.
TEST_F(gaia_references_test_t, erase_uninserted)
{
    auto_transaction_t txn;

    employee_writer employee_w;
    auto e1 = insert_employee(employee_w, "Horace");

    address_writer address_w;
    auto a1 = insert_address(address_w, "430 S. 41st St.", "Boulder");

    // The erase() should fail.
    EXPECT_THROW(e1.addressee_address_list().erase(a1), invalid_child);

    // Now insert it, erase, and erase again.
    e1.addressee_address_list().insert(a1);
    e1.addressee_address_list().erase(a1);
    EXPECT_THROW(e1.addressee_address_list().erase(a1), invalid_child);
}

// Make sure that erasing a member found in iterator doesn't crash.
TEST_F(gaia_references_test_t, erase_in_iterator)
{
    auto_transaction_t txn;

    employee_writer employee_w;
    auto e1 = insert_employee(employee_w, "Horace");

    address_writer address_w;
    auto a1 = insert_address(address_w, "430 S. 41st St.", "Boulder");
    auto a2 = insert_address(address_w, "10618 129th Pl. N.E.", "Kirkland");
    auto a3 = insert_address(address_w, "10805 Circle Dr.", "Bothell");

    e1.addressee_address_list().insert(a1);
    e1.addressee_address_list().insert(a2);
    e1.addressee_address_list().insert(a3);

    // We're happy with it not crashing, even though the list is cut short.
    int count = 0;
    for (auto a : e1.addressee_address_list())
    {
        e1.addressee_address_list().erase(a);
        count++;
    }
    EXPECT_EQ(count, 1);

    // There should be two on the list here, but same behavior.
    count = 0;
    for (auto a : e1.addressee_address_list())
    {
        e1.addressee_address_list().erase(a);
        count++;
    }
    EXPECT_EQ(count, 1);

    // Verify that one member remains.
    count = 0;
    for (auto const& a : e1.addressee_address_list())
    {
        EXPECT_STREQ(a.city(), "Boulder");
        count++;
    }
    EXPECT_EQ(count, 1);
}

// Scan beyond the end of the iterator.
TEST_F(gaia_references_test_t, scan_past_end)
{
    auto_transaction_t txn;

    employee_writer employee_w;
    auto e1 = insert_employee(employee_w, "Horace");

    address_writer address_w;
    auto a1 = insert_address(address_w, "430 S. 41st St.", "Boulder");
    auto a2 = insert_address(address_w, "10618 129th Pl. N.E.", "Kirkland");
    auto a3 = insert_address(address_w, "10805 Circle Dr.", "Bothell");

    e1.addressee_address_list().insert(a1);
    e1.addressee_address_list().insert(a2);
    e1.addressee_address_list().insert(a3);

    int count = 0;
    auto a = e1.addressee_address_list().begin();
    while (a != e1.addressee_address_list().end())
    {
        count++;
        a++;
    }
    EXPECT_EQ(count, 3);
    a++;
    EXPECT_EQ(a == e1.addressee_address_list().end(), true);
    a++;
    EXPECT_EQ(a == e1.addressee_address_list().end(), true);
}

// Attempt to insert two EDC objects in separate thread.
void insert_object(bool committed, employee_t e1, address_t a1)
{
    begin_session();
    begin_transaction();
    {
        if (committed)
        {
            e1.addressee_address_list().insert(a1);
        }
        else
        {
            // Nothing is committed yet.
            EXPECT_THROW(e1.addressee_address_list().insert(a1), edc_invalid_state);
        }
    }
    commit_transaction();
    end_session();
}

// Attempt to insert objects hydrated from IDs, in separate thread.
void insert_addressee(bool committed, gaia_id_t eid1, gaia_id_t aid1, gaia_id_t aid2, gaia_id_t aid3)
{
    begin_session();
    begin_transaction();
    {
        auto e1 = employee_t::get(eid1);
        auto a1 = address_t::get(aid1);
        auto a2 = address_t::get(aid2);
        auto a3 = address_t::get(aid3);
        if (committed)
        {
            // Note this first insert has already been done. This is no-op.
            e1.addressee_address_list().insert(a1);
            e1.addressee_address_list().insert(a2);
            e1.addressee_address_list().insert(a3);
        }
        else
        {
            EXPECT_THROW(e1.addressee_address_list().insert(a1), edc_invalid_state);
        }
    }
    commit_transaction();
    end_session();
}

// Create objects in one thread, connect them in another, verify in first thread.
TEST_F(gaia_references_test_t, thread_inserts)
{
    auto_transaction_t txn;

    employee_writer employee_w;
    auto e1 = insert_employee(employee_w, "Horace");

    address_writer address_w;
    auto a1 = insert_address(address_w, "430 S. 41st St.", "Boulder");
    auto a2 = insert_address(address_w, "10618 129th Pl. N.E.", "Kirkland");
    auto a3 = insert_address(address_w, "10805 Circle Dr.", "Bothell");

    // These threads should have problems since the objects aren't committed yet.
    thread t = thread(insert_object, false, e1, a1);
    t.join();

    t = thread(insert_addressee, false, e1.gaia_id(), a1.gaia_id(), a2.gaia_id(), a3.gaia_id());
    t.join();

    txn.commit();

    // Retry the threads after our objects are committed.
    t = thread(insert_object, true, e1, a1);
    t.join();
    t = thread(insert_addressee, true, e1.gaia_id(), a1.gaia_id(), a2.gaia_id(), a3.gaia_id());
    t.join();

    // Get a new transaction so we can get a new view of the employee instead of
    // the old view under the previous transaction. We will not be able to see
    // references updated in outside transactions.
    txn.commit();

    // Count the members. They should show up.
    int count = 0;
    for (auto a : e1.addressee_address_list())
    {
        count++;
    }
    EXPECT_EQ(count, 3);
}

// Testing the arrow dereference operator->() in gaia_set_iterator_t.
TEST_F(gaia_references_test_t, set_iter_arrow_deref)
{
    const char* emp_name = "Phillip";
    const char* addr_city = "Redmond";
    auto_transaction_t txn;

    employee_writer emp_writer;
    emp_writer.name_first = emp_name;
    employee_t employee = employee_t::get(emp_writer.insert_row());

    address_writer addr_writer;
    addr_writer.city = addr_city;

    employee.addressee_address_list().insert(addr_writer.insert_row());
    txn.commit();

    auto emp_addr_set_iter = employee.addressee_address_list().begin();
    EXPECT_STREQ(emp_addr_set_iter->city(), addr_city);
}

// Return true if an employee name includes an 'o'.
bool filter_function(const employee_t& e)
{
    string name(e.name_first());

    return name.find('o') != string::npos;
}

// Use various forms of filters on a set of references.
TEST_F(gaia_references_test_t, set_filter)
{
    auto_transaction_t txn;

    employee_writer writer;
    auto e_mgr = insert_employee(writer, "Harold");
    auto e_emp = insert_employee(writer, "Hunter");
    e_mgr.manages_employee_list().insert(e_emp);
    e_emp = insert_employee(writer, "Howard");
    e_mgr.manages_employee_list().insert(e_emp);
    e_emp = insert_employee(writer, "Henry");
    e_mgr.manages_employee_list().insert(e_emp);
    e_emp = insert_employee(writer, "Harry");
    e_mgr.manages_employee_list().insert(e_emp);
    e_emp = insert_employee(writer, "Hoover");
    e_mgr.manages_employee_list().insert(e_emp);

    size_t name_length = 5;
    int count = 0;
    auto name_length_list = e_mgr.manages_employee_list()
                                .where([&name_length](const employee_t& e) {
                                    return strlen(e.name_first()) == name_length;
                                });
    for (const auto& e : name_length_list)
    {
        EXPECT_EQ(strlen(e.name_first()), name_length);
        count++;
    }
    EXPECT_EQ(count, 2);
    name_length = 6;
    count = 0;
    auto it = name_length_list.begin();
    while (*it)
    {
        count++;
        ++it;
    }
    EXPECT_EQ(count, 3);

    count = 0;
    // Note that "Harold" is not counted because it is the owner.
    for (const auto& e : e_mgr.manages_employee_list().where(filter_function))
    {
        EXPECT_NE(strchr(e.name_first(), 'o'), nullptr);
        count++;
    }
    EXPECT_EQ(count, 2);
}

// Create a hierarchy
