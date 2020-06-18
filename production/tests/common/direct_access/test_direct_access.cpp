/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include "gtest/gtest.h"
#include "addr_book.h"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;
using namespace AddrBook;

class gaia_object_test : public ::testing::Test {
protected:
    void delete_employees() {
        Employee* e;
        begin_transaction();
        for(e = Employee::get_first();
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
            delete e;
        }
        commit_transaction();
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

int count_rows() {
    int count = 0;
    for (auto e = Employee::get_first();
         e;
         e = e->get_next())
    {
        ++count;
    }
    return count;
}

Employee* set_field(const char* name) {
    auto e = new Employee();
    e->set_name_first(name);
    return e;
}

Employee* insert_row(const char* name) {
    auto e = set_field(name);
    EXPECT_NE(e, nullptr);
    e->insert_row();
    return e;
}

Employee* update_row(const char* name) {
    auto e = insert_row(name);
    EXPECT_NE(e, nullptr);
    e->update_row();
    return e;
}

Employee* get_field(const char* name) {
    auto e = update_row(name);
    EXPECT_STREQ(e->name_first(), name);
    return e;
}

// Test on objects created by new()
// ================================

// Create, write & read, one row
TEST_F(gaia_object_test, get_field) {
    begin_transaction();
    get_field("Harold");
    commit_transaction();
}

// Delete one row
TEST_F(gaia_object_test, get_field_delete) {
    begin_transaction();
    auto e = get_field("Jameson");
    e->delete_row();
    delete e;
    commit_transaction();
}

// Scan multiple rows
TEST_F(gaia_object_test, new_set_ins) {
    begin_transaction();
    get_field("Harold");
    get_field("Jameson");
    EXPECT_EQ(2, count_rows());
    commit_transaction();
}

// Read back from new, unsaved object
TEST_F(gaia_object_test, net_set_get) {
    begin_transaction();
    auto e = new Employee();
    e->set_name_last("Smith");
    EXPECT_STREQ(e->name_last(), "Smith");
    EXPECT_STREQ(e->name_last_original(), "Smith");
    // note: no row inserted
    delete e;
    commit_transaction();
}

// Attempt to read original value from new object
TEST_F(gaia_object_test, read_original_from_copy) {
    begin_transaction();
    auto e = get_field("Zachary");
    EXPECT_STREQ("Zachary", e->name_first_original());
    EXPECT_STREQ("Zachary", e->name_first());
    commit_transaction();
}

// Insert a row with no field values
TEST_F(gaia_object_test, new_ins_get) {
    begin_transaction();
    
    auto e = new Employee();
    e->insert_row();
    auto name = e->name_first();
    auto hire_date = e->hire_date();

    EXPECT_EQ(name, nullptr);
    EXPECT_EQ(0, hire_date);

    commit_transaction();
}

// Delete an un-inserted object with field values
TEST_F(gaia_object_test, new_get) {
    begin_transaction();
    
    auto e = new Employee();
    auto name = e->name_first();
    auto hire_date = e->hire_date();

    EXPECT_EQ(name, nullptr);
    EXPECT_EQ(0, hire_date);
    delete e;

    commit_transaction();
}

TEST_F(gaia_object_test, new_del_field_ref) {
    // create GAIA-64 scenario
    begin_transaction();

    auto e = new Employee();
    e->insert_row();
    e->delete_row();
    auto name = e->name_first();
    EXPECT_EQ(name, nullptr);
    auto hire_date = e->hire_date();
    EXPECT_EQ(hire_date, 0);
    e->set_name_last("Hendricks");

    commit_transaction();
}

TEST_F(gaia_object_test, new_del_original_field_ref) {
    // create GAIA-64 scenario
    begin_transaction();

    auto e = new Employee();
    e->insert_row();
    e->delete_row();
    auto name = e->name_first_original();
    EXPECT_EQ(name, nullptr);
    auto hire_date = e->hire_date_original();
    EXPECT_EQ(hire_date, 0);

    commit_transaction();
}

// Test connecting, disconnecting, navigating records
// ==================================================
TEST_F(gaia_object_test, connect) {
    begin_transaction();

    // Connect two new rows.
    Employee* e1 = new Employee();
    Address* a1 = new Address();
    EXPECT_THROW(e1->addresss.insert(a1), edc_unstored_row);

    // Connect two non-inserted rows.
    Employee* e2 = new Employee();
    e2->set_name_first("Howard");
    Address* a2 = new Address();
    a2->set_city("Houston");
    EXPECT_THROW(e2->addresss.insert(a2), edc_unstored_row);

    // Connect two inserted rows.
    Employee* e3 = new Employee();
    e3->set_name_first("Hidalgo");
    Address* a3 = new Address();
    a3->set_city("Houston");
    e3->insert_row();
    a3->insert_row();
    e3->addresss.insert(a3);
    int count = 0;
    for (auto ap : e3->addresss) {
        if (ap) {
            count++;
        }
    }
    EXPECT_EQ(count, 1);
    delete e1;
    delete e2;
    delete e3;
    delete a1;
    delete a2;
    delete a3;
    commit_transaction();
}

Employee* create_hierarchy() {
    auto eptr = Employee::insert_row("Heidi", "Humphry", "555-22-4444", 20200530, "heidi@gmail.com", "");
    for (int i = 0; i<2000; i++) {
        char addr_string[6];
        sprintf(addr_string, "%d", i);
        auto aptr = Address::insert_row(addr_string, addr_string, addr_string, addr_string, addr_string, addr_string, true);
        eptr->addresss.insert(aptr);
        for (int j = 0; j < 40; j++) {
            char phone_string[5];
            sprintf(phone_string, "%d", j);
            auto pptr = Phone::insert_row(phone_string, phone_string, true);
            aptr->phones.insert(pptr);
        }
    }
    return eptr;
}

int scan_hierarchy(Employee* eptr) {
    int count = 1;
    for (auto aptr : eptr->addresss) {
        ++count;
        for (auto pptr : aptr->phones) {
            if (pptr) {
                ++count;
            }
        }
    }
    return count;
}

bool bounce_hierarchy(Employee* eptr) {
    // Take a subset of the hierarchy and travel to the bottom. From the bottom, travel back
    // up, verifying the results on the way.
    int count_addresses = 0;
    for (auto aptr : eptr->addresss) {
        if ((++count_addresses % 30) == 0) {
            int count_phones = 0;
            for (auto pptr : aptr->phones) {
                if ((++count_phones % 4) == 0) {
                    auto up_aptr = pptr->address();
                    EXPECT_EQ(up_aptr, aptr);
                    auto up_eptr = up_aptr->employee();
                    EXPECT_EQ(up_eptr, eptr);
                }
            }
        }
    }
    return true;
}

bool delete_hierarchy(Employee* eptr) {
    int count_addresses = 1;
    while (count_addresses>=1) {
        count_addresses = 0;
        // As long as there is at least one Address, continue
        Address* xaptr;
        for (auto aptr : eptr->addresss) {
            ++count_addresses;
            xaptr = aptr;
            // Repeat: delete the last phone until all are deleted
            int count_phones = 1;
            while (count_phones>=1) {
                count_phones = 0;
                Phone* xpptr;
                for (auto pptr : aptr->phones) {
                    ++count_phones;
                    xpptr = pptr;
                }
                if (count_phones) {
                    aptr->phones.erase(xpptr);
                    xpptr->delete_row();
                }
            }
        }
        if (count_addresses) {
            eptr->addresss.erase(xaptr);
            xaptr->delete_row();
        }
    }
    eptr->delete_row();
    return true;
}

TEST_F(gaia_object_test, connect_scan) {
    begin_transaction();

    // Create a hierarchy of employee to address to phone
    auto eptr = create_hierarchy();

    // Removing a row involved in any set should be prevented.
    EXPECT_THROW(eptr->delete_row(), node_not_disconnected);

    // Count the records in the hierarchy
    auto record_count = scan_hierarchy(eptr);
    EXPECT_EQ(record_count, 82001);

    // Travel down, then up the hierarchy
    EXPECT_EQ(bounce_hierarchy(eptr), true);

    // Delete the hierarchy, every third record, until it's gone
    EXPECT_EQ(delete_hierarchy(eptr), true);
    commit_transaction();
}

// Test on existing objects found by ID
// ====================================

// Create, write two rows, read back by ID and verify
TEST_F(gaia_object_test, read_back_id) {
    begin_transaction();
    auto eid = get_field("Howard")->gaia_id();
    auto eid2 = get_field("Henry")->gaia_id();
    commit_transaction();

    begin_transaction();
    auto e = Employee::get_row_by_id(eid);
    EXPECT_STREQ("Howard", e->name_first());
    EXPECT_STREQ("Howard", e->name_first_original());
    e = Employee::get_row_by_id(eid2);
    EXPECT_STREQ("Henry", e->name_first());
    EXPECT_STREQ("Henry", e->name_first_original());
    // Change the field and verify that original value is intact
    e->set_name_first("Heinrich");
    EXPECT_STREQ("Heinrich", e->name_first());
    EXPECT_STREQ("Henry", e->name_first_original());
    // While we have the original and modified values, update
    e->update_row();
    EXPECT_STREQ("Heinrich", e->name_first());
    EXPECT_STREQ("Henry", e->name_first_original());
    // Setting a field value after the update
    e->set_name_first("Hank");
    EXPECT_STREQ("Hank", e->name_first());
    EXPECT_STREQ("Henry", e->name_first_original());
    // Delete this object with original and modified fields
    e->delete_row();
    // The get, get_original and set should all success on this
    EXPECT_STREQ("Henry", e->name_first());
    EXPECT_STREQ("Henry", e->name_first_original());
    e->set_name_first("Harvey");
    // Finally, deleting this should be invalid
    EXPECT_THROW(e->delete_row(), invalid_node_id);
    commit_transaction();
    // Since e was deleted, this will not be cleaned up in TearDown
    delete e;
}

// Create row, try getting row from wrong type
TEST_F(gaia_object_test, read_wrong_type) {
    begin_transaction();
    auto eid = get_field("Howard")->gaia_id();
    commit_transaction();

    begin_transaction();
    try {
        Address::get_row_by_id(eid);
    }
    catch (const exception& e) {
        // The eid is unpredictable, but the exception will use it in its message.
        string compare_string = "Requesting Gaia type Address(2) but object identified by " + to_string(eid) + " is type Employee(1).";
        EXPECT_STREQ(e.what(), compare_string.c_str());
    }
    EXPECT_THROW(Address::get_row_by_id(eid), edc_invalid_object_type);
    commit_transaction();
}

// Create, write two rows, read back by scan and verify
TEST_F(gaia_object_test, read_back_scan) {
    begin_transaction();
    auto eid = get_field("Howard")->gaia_id();
    auto eid2 = get_field("Henry")->gaia_id();
    commit_transaction();

    begin_transaction();
    auto e = Employee::get_first();
    EXPECT_EQ(eid, e->gaia_id());
    EXPECT_STREQ("Howard", e->name_first());
    EXPECT_STREQ("Howard", e->name_first_original());
    e = e->get_next();
    EXPECT_EQ(eid2, e->gaia_id());
    EXPECT_STREQ("Henry", e->name_first());
    EXPECT_STREQ("Henry", e->name_first_original());
    commit_transaction();
}

// Used twice, below
void UpdateReadBack(bool update_flag) {
    begin_transaction();
    get_field("Howard");
    get_field("Henry");
    commit_transaction();

    begin_transaction();
    auto e = Employee::get_first();
    e->set_name_first("Herald");
    e->set_name_last("Hollman");
    if (update_flag) {
        e->update_row();
    }
    e = e->get_next();
    e->set_name_first("Gerald");
    e->set_name_last("Glickman");
    if (update_flag) {
        e->update_row();
    }
    commit_transaction();

    begin_transaction();
    e = Employee::get_first();
    if (update_flag) {
        EXPECT_STREQ("Herald", e->name_first());
        EXPECT_STREQ("Hollman", e->name_last());
    } else {
        // unchanged by previous transaction
        EXPECT_STREQ("Howard", e->name_first());
        EXPECT_STREQ(nullptr, e->name_last());
    }
    e = e->get_next();
    if (update_flag) {
        EXPECT_STREQ("Gerald", e->name_first());
        EXPECT_STREQ("Glickman", e->name_last());
    } else {
        // unchanged by previous transaction
        EXPECT_STREQ("Henry", e->name_first());
        EXPECT_STREQ(nullptr, e->name_last());
    }
    commit_transaction();
}

// Create, write two rows, set fields, update, read, verify
TEST_F(gaia_object_test, update_read_back) {
    UpdateReadBack(true);
}

// Create, write two rows, set fields, update, read, verify
TEST_F(gaia_object_test, no_update_read_back) {
    UpdateReadBack(false);
}

// Delete an inserted object then insert after, it's good again
TEST_F(gaia_object_test, new_del_ins) {
    begin_transaction();
    auto e = get_field("Hector");
    e->delete_row();
    e->insert_row();
    e->set_name_first("Hudson");
    if (e != nullptr) {
        delete e;
    }
    commit_transaction();
}

// Delete a found object then update
TEST_F(gaia_object_test, new_del_upd) {
    begin_transaction();
    auto e = get_field("Hector");
    e->delete_row();
    e->update_row();
    commit_transaction();
    delete e;
}

// Delete a found object then insert after, it's good again
TEST_F(gaia_object_test, found_del_ins) {
    begin_transaction();
    
    auto e = get_field("Hector");
    e->delete_row();
    e->insert_row();
    auto eid = e->gaia_id();
    commit_transaction();

    begin_transaction();
    e = Employee::get_row_by_id(eid);
    e->set_name_first("Hudson");
    if (e != nullptr) {
        delete e;
    }
    commit_transaction();

}

// Delete an inserted object then set field after, it's good again
TEST_F(gaia_object_test, new_del_set) {
    begin_transaction();
    auto e = get_field("Hector");
    e->delete_row();
    e->set_name_first("Howard");
    if (e != nullptr) {
        delete e;
    }
    commit_transaction();

}

// Delete a found object then update
TEST_F(gaia_object_test, found_del_upd) {
    begin_transaction();
    auto e = get_field("Hector");
    auto eid = e->gaia_id();
    commit_transaction();

    begin_transaction();
    e = Employee::get_row_by_id(eid);
    e->delete_row();
    e->update_row();
    if (e != nullptr) {
        delete e;
    }
    commit_transaction();

}

// EXCEPTION conditions
// ====================

// Attempt to insert one row twice
TEST_F(gaia_object_test, insert_x2) {
    begin_transaction();
    auto e = get_field("Zachary");
    EXPECT_THROW(e->insert_row(), duplicate_id);
    commit_transaction();
}

// Create, then get by ID, insert
// Used twice, below
void InsertIdX2(bool insert_flag) {
    begin_transaction();
    auto eid = get_field("Zachary")->gaia_id();
    commit_transaction();

    begin_transaction();
    auto e = Employee::get_row_by_id(eid);
    if (insert_flag) {
        e->set_name_first("Zerubbabel");
    }
    EXPECT_THROW(e->insert_row(), duplicate_id);
    commit_transaction();
}

// Attempt to insert a row found by ID
TEST_F(gaia_object_test, insert_id_x2) {
    InsertIdX2(false);
}

// Attempt to insert a row found by ID after setting field value
TEST_F(gaia_object_test, set_insert_id_x2) {
    InsertIdX2(true);
}

// Attempt to create a row outside of a transaction
TEST_F(gaia_object_test, no_tx) {
    EXPECT_THROW(get_field("Harold"), tx_not_open);
    // NOTE: the Employee object is leaked here
}

// Attempt to update an un-inserted object
TEST_F(gaia_object_test, new_upd) {
    begin_transaction();
    Employee* emp = new Employee();
    emp->set_name_first("Judith");
    EXPECT_THROW(emp->update_row(), invalid_node_id);
    if (emp != nullptr) {
        delete emp;
    }
    //Employee* emp2 = new Employee(0);
    //printf("%s\n", emp2->name_first());
    commit_transaction();
}

// Delete an un-inserted object
TEST_F(gaia_object_test, new_del) {
    begin_transaction();
    auto e = new Employee();
    EXPECT_THROW(e->delete_row(), invalid_node_id);
    if (e != nullptr) {
        delete e;
    }
    commit_transaction();
}

// Delete a row twice
TEST_F(gaia_object_test, new_del_del) {
    begin_transaction();
    auto e = get_field("Hugo");
    // the first delete succeeds
    e->delete_row();
    // second one fails
    EXPECT_THROW(e->delete_row(), invalid_node_id);
    commit_transaction();
    delete e;
}

// Perform get_next() without a preceeding get_first()
TEST_F(gaia_object_test, next_first) {
    begin_transaction();
    auto e1 = get_field("Harold");
    auto e2 = get_field("Howard");
    auto e3 = get_field("Hank");
    auto e_test = e2->get_next();
    EXPECT_TRUE(e_test == e1 || e_test == e3 || e_test == nullptr);
    auto e4 = new Employee();
    // In this case, the row doesn't exist yet.
    e4->set_name_first("Hector");
    EXPECT_EQ(nullptr, e4->get_next());
    commit_transaction();
}
