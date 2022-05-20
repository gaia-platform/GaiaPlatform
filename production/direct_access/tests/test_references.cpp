/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <charconv>
#include <iostream>

#include "gtest/gtest.h"

#include <gaia/direct_access/auto_transaction.hpp>
#include <gaia/exceptions.hpp>

#include "gaia_internal/db/db_catalog_test_base.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"
#include "gaia_internal/db/gaia_relationships.hpp"

#include "gaia_addr_book.h"

using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::addr_book;

using std::string;
using std::thread;
using std::to_string;

class direct_access__references__test : public db_catalog_test_base_t
{
public:
    inline static constexpr size_t hire_date = 20200530;
    inline static constexpr size_t count_employees = 1;
    inline static constexpr size_t count_addresses = 200;
    inline static constexpr size_t count_phones = 20;
    inline static constexpr size_t addr_size = 6;
    inline static constexpr size_t phone_size = 5;

protected:
    direct_access__references__test()
        : db_catalog_test_base_t("addr_book.ddl", true, true, true)
    {
    }

    static gaia_id_t find_invalid_id()
    {
        // Starting from an high ID to reduce the chances of collision
        // with already existent ids.
        const int c_lower_id_range = 10000;

        // 1M oughta to be enough.
        const int c_higher_id_range = 10000 * 100;

        for (int i = c_lower_id_range; i < c_higher_id_range; i++)
        {
            auto invalid_obj = gaia_ptr_t::from_gaia_id(i);

            if (!invalid_obj)
            {
                return i;
            }
        }

        throw std::runtime_error(
            "Cannot find an invalid ID in the range "
            + to_string(c_lower_id_range) + " - " + to_string(c_higher_id_range));
    }

    employee_t insert_records(size_t count)
    {
        employee_writer employee_writer;
        address_writer address_writer;

        employee_writer.name_first = "Many";
        employee_writer.name_last = "Addresses";
        employee_t employee = employee_t::get(employee_writer.insert_row());

        for (size_t i = 0; i < count; i++)
        {
            address_writer.street = to_string(i);
            gaia_id_t address_id = address_writer.insert_row();
            employee.addresses().insert(address_id);
        }

        return employee;
    }

    employee_t insert_employee(const std::string& name)
    {
        employee_writer ew;
        ew.name_first = name.c_str();
        return employee_t::get(ew.insert_row());
    }

    address_t insert_address(const std::string& street, const std::string& city)
    {
        address_writer aw;
        aw.city = city.c_str();
        aw.street = street.c_str();
        return address_t::get(aw.insert_row());
    }
};

// Test connecting, disconnecting, navigating records
// ==================================================
TEST_F(direct_access__references__test, insert_remove)
{
    begin_transaction();

    // Connect two inserted rows.
    employee_writer ew;
    ew.name_first = "Hidalgo";
    employee_t e3 = employee_t::get(ew.insert_row());

    address_writer aw;
    aw.city = "Houston";
    address_t a3 = address_t::get(aw.insert_row());

    e3.addresses().insert(a3);

    EXPECT_EQ(e3.addresses().size(), 1);

    e3.addresses().remove(a3);
    EXPECT_EQ(e3.addresses().size(), 0);
    commit_transaction();
}

TEST_F(direct_access__references__test, connect_disconnect)
{
    begin_transaction();

    // Connect two inserted rows.
    employee_t e1 = insert_employee("Hidalgo");
    address_t a1 = insert_address("2400 4th Ave", "Houston");
    e1.addresses().connect(a1);
    EXPECT_EQ(e1.addresses().size(), 1);

    e1.addresses().disconnect(a1);
    EXPECT_EQ(e1.addresses().size(), 0);
    commit_transaction();
}

// Repeat above test, but with gaia_id_t members only.
TEST_F(direct_access__references__test, insert_remove_id_member)
{
    begin_transaction();

    // Connect two inserted rows.
    employee_writer ew;
    ew.name_first = "Hidalgo";
    employee_t e3 = employee_t::get(ew.insert_row());

    address_writer aw;
    aw.city = "Houston";
    gaia_id_t aid3 = aw.insert_row();

    e3.addresses().insert(aid3);
    int count = 0;
    for (auto const& ap : e3.addresses())
    {
        if (ap)
        {
            count++;
        }
    }
    EXPECT_EQ(count, 1);

    gaia_id_t invalid_id = find_invalid_id();

    e3.addresses().remove(aid3);
    address_t::delete_row(aid3);
    e3.delete_row();
    EXPECT_THROW(address_t::delete_row(invalid_id), invalid_object_id);
    commit_transaction();
}

TEST_F(direct_access__references__test, connect_disconnect_id_member)
{
    begin_transaction();

    // Connect two inserted rows.
    employee_t e1 = insert_employee("Hidalgo");
    address_t a1 = insert_address("2400 4th Ave", "Houston");
    e1.addresses().connect(a1.gaia_id());
    EXPECT_EQ(e1.addresses().size(), 1);

    e1.addresses().disconnect(a1.gaia_id());
    EXPECT_EQ(e1.addresses().size(), 0);
    commit_transaction();
}

employee_t create_hierarchy()
{

    auto employee
        = employee_t::get(employee_t::insert_row("Heidi", "Humphry", "555-22-4444", direct_access__references__test::hire_date, "heidi@gmail.com", ""));
    for (size_t i = 0; i < direct_access__references__test::count_addresses; i++)
    {
        char addr_string[direct_access__references__test::addr_size];
        sprintf(addr_string, "%zu", i);
        auto address = address_t::get(
            address_t::insert_row(addr_string, addr_string, addr_string, addr_string, addr_string, addr_string, true));
        employee.addresses().insert(address);
        for (size_t j = 0; j < direct_access__references__test::count_phones; j++)
        {
            char phone_string[direct_access__references__test::phone_size];
            sprintf(phone_string, "%zu", j);
            auto phone = phone_t::get(phone_t::insert_row(phone_string, phone_string, true));
            address.phones().insert(phone);
        }
    }
    return employee;
}

int scan_hierarchy(employee_t& eptr)
{
    int count = 1;
    for (const auto& aptr : eptr.addresses())
    {
        ++count;
        for (auto const& pptr : aptr.phones())
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
    for (const auto& aptr : eptr.addresses())
    {
        if ((++count_addressee % c_count_subset) == 0)
        {
            int count_phones = 0;
            for (const auto& pptr : aptr.phones())
            {
                if ((++count_phones % 4) == 0)
                {
                    auto up_aptr = pptr.address();
                    EXPECT_EQ(up_aptr, aptr);
                    auto up_eptr = up_aptr.owner();
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
        for (auto& address : employee_to_delete.addresses())
        {
            ++count_addressee;
            address_to_delete = address;
            // Repeat: delete the last phone until all are deleted
            int count_phones = 1;
            while (count_phones >= 1)
            {
                count_phones = 0;

                phone_t phone_to_delete;
                for (const auto& phone : address.phones())
                {
                    ++count_phones;
                    phone_to_delete = phone;
                }

                if (count_phones)
                {
                    address.phones().remove(phone_to_delete);
                    phone_to_delete.delete_row();
                }
            }
        }
        if (count_addressee)
        {
            employee_to_delete.addresses().remove(address_to_delete);
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
    return employee_t::list().begin()->name_first();
}

int all_addressee()
{
    // This is the sum from 0 to N-1, with N=count_addresses.
    auto count_addresses_sum
        = ((direct_access__references__test::count_addresses
            * (direct_access__references__test::count_addresses + 1))
           / 2)
        - direct_access__references__test::count_addresses;

    // Decrement the sum of all addresses' numeric values by the current
    // address's numeric value.
    for (auto const& address : address_t::list())
    {
        auto str = address.city();
        size_t value{};
        std::from_chars(str, str + ::strlen(str), value);
        count_addresses_sum -= value;
    }

    return count_addresses_sum;
}

// Create a hierachy of records, then scan and count them.
TEST_F(direct_access__references__test, connect_scan)
{
    begin_transaction();

    // Create a hierarchy of employee to address to phone
    auto eptr = create_hierarchy();

    // Removing a row involved in any set should be prevented.
    EXPECT_THROW(eptr.delete_row(), object_still_referenced);

    // Count the records in the hierarchy
    auto record_count = scan_hierarchy(eptr);
    auto total_count_addresses = direct_access__references__test::count_employees * direct_access__references__test::count_addresses;
    auto total_count_phones = total_count_addresses * direct_access__references__test::count_phones;
    EXPECT_EQ(record_count, direct_access__references__test::count_employees + total_count_addresses + total_count_phones);

    // Travel down, then up the hierarchy
    EXPECT_EQ(bounce_hierarchy(eptr), true);

    // Count the rows.
    EXPECT_EQ(count_type<employee_t>(), direct_access__references__test::count_employees);
    EXPECT_EQ(count_type<address_t>(), total_count_addresses);
    EXPECT_EQ(count_type<phone_t>(), total_count_phones);

    // Scan through some rows.
    EXPECT_EQ(first_employee(), "Heidi");

    // Scan through all addresses.
    EXPECT_EQ(all_addressee(), 0);

    // Delete the hierarchy, every third record, until it's gone
    EXPECT_EQ(delete_hierarchy(eptr), true);
    commit_transaction();
}

void scan_manages(std::vector<string>& employee_vector, employee_t& e)
{
    employee_vector.emplace_back(e.name_first());
    for (auto eptr : e.reportees())
    {
        scan_manages(employee_vector, eptr);
    }
}

// Test recursive scanning, employee_t to employee_t through manages relationship.
TEST_F(direct_access__references__test, recursive_scan)
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

    auto e1 = insert_employee("Horace");
    auto e2 = insert_employee("Henry");
    auto e3 = insert_employee("Hal");
    auto e4 = insert_employee("Hiram");
    auto e5 = insert_employee("Howard");
    auto e6 = insert_employee("Hector");
    auto e7 = insert_employee("Hank");

    e1.reportees().insert(e2); // Horace to Henry
    e2.reportees().insert(e3); //    Henry to Hal
    e2.reportees().insert(e4); //    Henry to Hiram
    e4.reportees().insert(e5); //       Hiram to Howard
    e1.reportees().insert(e6); // Horace to Hector
    e1.reportees().insert(e7); // Horace to Hank

    // Recursive walk through hierarchy
    std::vector<string> employee_vector;
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
TEST_F(direct_access__references__test, connect_to_ids)
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
    e1.addresses().insert(a1);
    e1.addresses().insert(a2);
}

// Connect objects created in prior transaction.
TEST_F(direct_access__references__test, connect_after_txn)
{
    auto_transaction_t txn;

    auto e1 = insert_employee("Horace");
    auto a1 = insert_address("430 S. 41st St.", "Boulder");
    auto a2 = insert_address("10618 129th Pl. N.E.", "Kirkland");

    // In a subsequent transaction, connect the objects.
    txn.commit();

    // Use the objects from the previous transaction.
    e1.addresses().insert(a1);
    e1.addresses().insert(a2);
    auto addr = e1.addresses().begin();
    EXPECT_STREQ((*addr).city(), "Kirkland");
    ++addr;
    EXPECT_STREQ((*addr).city(), "Boulder");
}

// Remove list members inserted in prior transaction.
TEST_F(direct_access__references__test, disconnect_after_txn)
{
    auto_transaction_t txn;

    auto e1 = insert_employee("Horace");
    auto a1 = insert_address("430 S. 41st St.", "Boulder");
    auto a2 = insert_address("10618 129th Pl. N.E.", "Kirkland");

    e1.addresses().insert(a1);
    e1.addresses().insert(a2);

    // In a subsequent transaction, disconnect the objects.
    txn.commit();

    e1.addresses().remove(a1);
    e1.addresses().remove(a2);
}

// Generate an exception by attempting to insert member twice.
TEST_F(direct_access__references__test, connect_twice)
{
    auto_transaction_t txn;

    /* Create some unconnected Employee and Address objects */
    auto e1 = insert_employee("Horace");
    auto e2 = insert_employee("Hector");
    auto a1 = insert_address("430 S. 41st St.", "Boulder");

    // The second insert is redundant and is treated as a no-op.
    e1.addresses().insert(a1);
    e1.addresses().insert(a1);

    // The third insert is illegal because the address cannot be on the
    // same list of two owners.
    EXPECT_THROW(e2.addresses().insert(a1), child_already_referenced);
}

// Generate an exception by attempting to remove un-inserted member.
TEST_F(direct_access__references__test, remove_uninserted)
{
    auto_transaction_t txn;

    auto e1 = insert_employee("Horace");
    auto a1 = insert_address("430 S. 41st St.", "Boulder");

    // The remove should not have any effect.
    e1.addresses().remove(a1);

    // Now insert it, remove, and remove again.
    e1.addresses().insert(a1);
    e1.addresses().remove(a1);

    // The remove should not have any effect.
    e1.addresses().remove(a1);
}

// Make sure that erasing a member found in iterator doesn't crash.
TEST_F(direct_access__references__test, remove_in_iterator)
{
    auto_transaction_t txn;

    auto e1 = insert_employee("Horace");

    auto a1 = insert_address("430 S. 41st St.", "Boulder");
    auto a2 = insert_address("10618 129th Pl. N.E.", "Kirkland");
    auto a3 = insert_address("10805 Circle Dr.", "Bothell");

    e1.addresses().insert(a1);
    e1.addresses().insert(a2);
    e1.addresses().insert(a3);

    // We're happy with it not crashing, even though the list is cut short.
    int count = 0;
    for (const auto& a : e1.addresses())
    {
        e1.addresses().remove(a);
        count++;
    }
    EXPECT_EQ(count, 1);

    // There should be two on the list here, but same behavior.
    count = 0;
    for (const auto& a : e1.addresses())
    {
        e1.addresses().remove(a);
        count++;
    }
    EXPECT_EQ(count, 1);

    // Verify that one member remains.
    count = 0;
    for (auto const& a : e1.addresses())
    {
        EXPECT_STREQ(a.city(), "Boulder");
        count++;
    }
    EXPECT_EQ(count, 1);
}

// Scan beyond the end of the iterator.
TEST_F(direct_access__references__test, scan_past_end)
{
    auto_transaction_t txn;

    auto e1 = insert_employee("Horace");

    auto a1 = insert_address("430 S. 41st St.", "Boulder");
    auto a2 = insert_address("10618 129th Pl. N.E.", "Kirkland");
    auto a3 = insert_address("10805 Circle Dr.", "Bothell");

    e1.addresses().insert(a1);
    e1.addresses().insert(a2);
    e1.addresses().insert(a3);

    int count = 0;
    auto a = e1.addresses().begin();
    while (a != e1.addresses().end())
    {
        count++;
        a++;
    }
    EXPECT_EQ(count, 3);
    a++;
    EXPECT_EQ(a == e1.addresses().end(), true);
    a++;
    EXPECT_EQ(a == e1.addresses().end(), true);
}

// Attempt to insert two DAC objects in separate thread.
void insert_object(bool committed, employee_t e1, address_t a1)
{
    begin_session();
    begin_transaction();
    {
        if (committed)
        {
            e1.addresses().insert(a1);
        }
        else
        {
            // Nothing is committed yet.
            EXPECT_THROW(e1.addresses().insert(a1), invalid_object_id);
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
        if (committed)
        {
            auto e1 = employee_t::get(eid1);
            auto a1 = address_t::get(aid1);
            auto a2 = address_t::get(aid2);
            auto a3 = address_t::get(aid3);

            // Note this first insert has already been done. This is no-op.
            e1.addresses().insert(a1);
            e1.addresses().insert(a2);
            e1.addresses().insert(a3);
        }
        else
        {
            EXPECT_THROW(employee_t::get(eid1), invalid_object_id);
            EXPECT_THROW(address_t::get(aid1), invalid_object_id);
            EXPECT_THROW(address_t::get(aid2), invalid_object_id);
            EXPECT_THROW(address_t::get(aid3), invalid_object_id);
        }
    }
    commit_transaction();
    end_session();
}

// Connect and scan a many-to-many relationships through phone_t.
TEST_F(direct_access__references__test, m_to_n_connections)
{
    auto_transaction_t txn;

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    auto e1 = employee_t::get(employee_t::insert_row("Hubert", "Humphrey", "XXX", 1902, "", ""));
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    auto e2 = employee_t::get(employee_t::insert_row("Howard", "Hughs", "YYY", 1895, "", ""));
    auto a1 = address_t::get(address_t::insert_row("1233", "", "Bot Hell", "98099", "AW", "USA", false));
    auto a2 = address_t::get(address_t::insert_row("11111", "", "LandofKirk", "89088", "OW", "USA", false));
    auto p1 = phone_t::get(phone_t::insert_row("303", "H", false));
    auto p2 = phone_t::get(phone_t::insert_row("303", "M", false));
    auto p3 = phone_t::get(phone_t::insert_row("206", "H", false));
    auto p4 = phone_t::get(phone_t::insert_row("206", "M", false));
    e1.phones().insert(p1);
    e1.phones().insert(p2);
    e2.phones().insert(p3);
    e2.phones().insert(p4);
    a1.phones().insert(p1);
    a1.phones().insert(p3);
    a2.phones().insert(p2);
    a2.phones().insert(p4);

    // Scanning from e1 to (p1 to a1) to (p2 to a2).
    for (auto& p : e1.phones())
    {
        auto a = p.address();
        EXPECT_EQ(strcmp("303", p.phone_number()), 0);
        if (strcmp(p.type(), "H") == 0)
        {
            EXPECT_EQ(strcmp("1233", a.street()), 0);
        }
        else
        {
            EXPECT_EQ(strcmp("11111", a.street()), 0);
        }
    }

    // Scanning from e2 to (p3 to a1) to (p4 to a2).
    for (auto& p : e2.phones())
    {
        auto a = p.address();
        EXPECT_EQ(strcmp("206", p.phone_number()), 0);
        if (strcmp(p.type(), "H") == 0)
        {
            EXPECT_EQ(strcmp("1233", a.street()), 0);
        }
        else
        {
            EXPECT_EQ(strcmp("11111", a.street()), 0);
        }
    }

    // Scanning from a1 to (p1 to e1) to (p3 to e2).
    for (auto& p : a1.phones())
    {
        auto e = p.owner();
        if (strcmp(p.phone_number(), "303") == 0)
        {
            EXPECT_EQ(strcmp("Hubert", e.name_first()), 0);
        }
        else
        {
            EXPECT_EQ(strcmp("Howard", e.name_first()), 0);
        }
    }

    // Scanning from a2 to (p2 to e1) to (p4 to e2).
    for (auto& p : a2.phones())
    {
        auto e = p.owner();
        if (strcmp(p.phone_number(), "206") == 0)
        {
            EXPECT_EQ(strcmp("Howard", e.name_first()), 0);
        }
        else
        {
            EXPECT_EQ(strcmp("Hubert", e.name_first()), 0);
        }
    }
}

// Create objects in one thread, connect them in another, verify in first thread.
TEST_F(direct_access__references__test, thread_inserts)
{
    auto_transaction_t txn;

    auto e1 = insert_employee("Horace");

    auto a1 = insert_address("430 S. 41st St.", "Boulder");
    auto a2 = insert_address("10618 129th Pl. N.E.", "Kirkland");
    auto a3 = insert_address("10805 Circle Dr.", "Bothell");

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
    for (auto a : e1.addresses())
    {
        count++;
    }
    EXPECT_EQ(count, 3);
}

// Testing the arrow dereference operator->() in dac_set_iterator_t.
TEST_F(direct_access__references__test, set_iter_arrow_deref)
{
    const char* emp_name = "Phillip";
    const char* addr_city = "Redmond";
    auto_transaction_t txn;

    employee_writer emp_writer;
    emp_writer.name_first = emp_name;
    employee_t employee = employee_t::get(emp_writer.insert_row());

    address_writer addr_writer;
    addr_writer.city = addr_city;

    employee.addresses().insert(addr_writer.insert_row());
    txn.commit();

    auto emp_addr_set_iter = employee.addresses().begin();
    EXPECT_STREQ(emp_addr_set_iter->city(), addr_city);
}

// Return true if an employee name includes an 'o'.
bool filter_function(const employee_t& e)
{
    string name(e.name_first());

    return name.find('o') != string::npos;
}

// Use various forms of filters on a set of references.
TEST_F(direct_access__references__test, set_filter)
{
    auto_transaction_t txn;

    auto e_mgr = insert_employee("Harold");
    auto e_emp = insert_employee("Hunter");
    e_mgr.reportees().insert(e_emp);
    e_emp = insert_employee("Howard");
    e_mgr.reportees().insert(e_emp);
    e_emp = insert_employee("Henry");
    e_mgr.reportees().insert(e_emp);
    e_emp = insert_employee("Harry");
    e_mgr.reportees().insert(e_emp);
    e_emp = insert_employee("Hoover");
    e_mgr.reportees().insert(e_emp);

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    size_t name_length = 5;
    int count = 0;
    auto name_length_list = e_mgr.reportees()
                                .where([&name_length](const employee_t& e) { return strlen(e.name_first()) == name_length; });
    for (const auto& e : name_length_list)
    {
        EXPECT_EQ(strlen(e.name_first()), name_length);
        count++;
    }
    EXPECT_EQ(count, 2);
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
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
    for (const auto& e : e_mgr.reportees().where(filter_function))
    {
        EXPECT_NE(strchr(e.name_first(), 'o'), nullptr);
        count++;
    }
    EXPECT_EQ(count, 2);
}

TEST_F(direct_access__references__test, test_remove)
{
    begin_transaction();
    const size_t c_num_addresses = 10;

    employee_t employee = insert_records(c_num_addresses);
    auto addr_list = employee.addresses();

    for (const address_t& addr : addr_list)
    {
        addr_list.remove(addr);
    }

    // The line above should remove only one element from the container
    ASSERT_EQ(c_num_addresses - 1, std::distance(addr_list.begin(), addr_list.end()));

    auto addr_it = addr_list.begin();
    while (addr_it != addr_list.end())
    {
        addr_list.remove(*addr_it++);
    }

    ASSERT_EQ(addr_list.begin(), addr_list.end());

    commit_transaction();
}

TEST_F(direct_access__references__test, test_erase)
{
    begin_transaction();
    const size_t c_num_addresses = 10;

    employee_t employee = insert_records(c_num_addresses);
    auto addr_list = employee.addresses();

    for (auto addr_it = addr_list.begin(); addr_it != addr_list.end();)
    {
        addr_it = addr_list.erase(addr_it);
    }

    ASSERT_EQ(addr_list.begin(), addr_list.end());

    commit_transaction();
}

TEST_F(direct_access__references__test, test_erase_invalid_child)
{
    begin_transaction();
    const size_t c_num_addresses = 10;

    employee_t employee = insert_records(c_num_addresses);
    auto addr_list = employee.addresses();

    EXPECT_THROW(addr_list.erase(addr_list.end()), invalid_object_id);

    ASSERT_EQ(c_num_addresses, std::distance(addr_list.begin(), addr_list.end()));

    commit_transaction();
}

TEST_F(direct_access__references__test, test_clear)
{
    begin_transaction();
    const size_t c_num_addresses = 10;

    employee_t employee = insert_records(c_num_addresses);
    auto addr_list = employee.addresses();

    addr_list.clear();

    ASSERT_EQ(addr_list.begin(), addr_list.end());

    // Ensures that nothing happens.
    addr_list.clear();

    commit_transaction();
}

TEST_F(direct_access__references__test, test_dac_container_size)
{
    begin_transaction();

    ASSERT_EQ(0, employee_t::list().size());

    const size_t c_num_employees = 11;
    const size_t c_num_addresses = 1;

    for (size_t i = 0; i < c_num_employees; i++)
    {
        insert_records(c_num_addresses);
    }

    ASSERT_EQ(c_num_employees, employee_t::list().size());

    commit_transaction();
}

TEST_F(direct_access__references__test, test_refernece_container_size)
{
    begin_transaction();

    employee_t employee = insert_records(0);

    ASSERT_EQ(0, employee.addresses().size());

    const size_t c_num_addresses = 11;

    for (size_t i = 0; i < c_num_addresses; i++)
    {
        address_writer address_w;
        address_w.street = to_string(i);
        gaia_id_t address_id = address_w.insert_row();
        employee.addresses().insert(address_id);
    }

    ASSERT_EQ(c_num_addresses, employee.addresses().size());

    employee.addresses().clear();

    ASSERT_EQ(0, employee.addresses().size());

    commit_transaction();
}

TEST_F(direct_access__references__test, test_temporary_object)
{
    begin_transaction();

    // Connect two inserted rows.
    employee_writer ew;
    ew.name_first = "Hidalgo";
    employee_t emp = employee_t::get(ew.insert_row());

    // Ensure that we can pass a temporary object to the insert method.
    emp.addresses().insert(address_t::get(address_t::insert_row("", "", "", "", "", "", true)));

    commit_transaction();
}

TEST_F(direct_access__references__test, test_delete_referenced_child)
{
    auto_transaction_t txn;

    const size_t c_num_addresses = 10;
    employee_t employee = insert_records(c_num_addresses);
    txn.commit();

    size_t count = 0;
    for (auto addr = employee.addresses().begin(); addr != employee.addresses().end();)
    {
        auto prev = addr++;
        if (++count % 2 == 0)
        {
            // Referenced child objects can be deleted.
            ASSERT_NO_THROW(prev->delete_row());
        }
        else
        {
            // The 'force' delete option is not needed here. We use force
            // deletion for test completeness.
            ASSERT_NO_THROW(prev->delete_row(true));
        }
    }
    ASSERT_EQ(count, c_num_addresses);
    txn.commit();

    ASSERT_EQ(employee.addresses().size(), 0);
    address_t addr = insert_address("2400 4th Ave", "Houston");
    employee.addresses().connect(addr);
    txn.commit();

    ASSERT_EQ(employee.addresses().begin()->gaia_id(), addr.gaia_id());
    ASSERT_NO_THROW(addr.delete_row());
}

TEST_F(direct_access__references__test, test_delete_referenced_parent)
{
    auto_transaction_t txn;

    const size_t c_num_addresses = 10;
    employee_t employee = insert_records(c_num_addresses);

    std::array<address_t, c_num_addresses> addresses;
    std::copy(employee.addresses().begin(), employee.addresses().end(), addresses.begin());
    txn.commit();

    // Referenced parent object in an explicit 1:N relationships cannot be
    // deleted without the force option.
    ASSERT_THROW(employee.delete_row(), object_still_referenced);
    txn.commit();

    // Referenced parent object in explicit 1:N relationships can still be
    // deleted using the force option.
    ASSERT_NO_THROW(employee.delete_row(true));
    txn.commit();

    employee_t employee1 = insert_employee("e1");
    employee_t employee2 = insert_employee("e2");
    size_t count = 0;
    for (const auto& addr : addresses)
    {
        // All child objects will be disconnected from the force deleted parent
        // object. They can still be connected to other objects.
        ASSERT_FALSE(addr.owner());
        if (++count % 2 == 0)
        {
            employee1.addresses().insert(addr);
        }
        else
        {
            employee2.addresses().connect(addr);
        }
    }
    txn.commit();

    ASSERT_EQ(employee1.addresses().size(), c_num_addresses / 2);
    ASSERT_EQ(employee2.addresses().size(), c_num_addresses / 2);
}
