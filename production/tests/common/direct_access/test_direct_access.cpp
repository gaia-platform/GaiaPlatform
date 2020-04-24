/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include "gtest/gtest.h"
#include "addr_book_gaia_generated.h"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;
using namespace AddrBook;

class gaia_object_test : public ::testing::Test {
protected:
    void delete_employees() {
        Employee* e;
        gaia_base_t::begin_transaction();
        for(e = Employee::get_first();
            e;
            e = Employee::get_first())
        {
            e->delete_row();
            delete e;
        }
        gaia_base_t::commit_transaction();
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
    gaia_base_t::begin_transaction();
    get_field("Harold");
    gaia_base_t::commit_transaction();
}

// Delete one row
TEST_F(gaia_object_test, get_field_delete) {
    gaia_base_t::begin_transaction();
    auto e = get_field("Jameson");
    e->delete_row();
    delete e;
    gaia_base_t::commit_transaction();
}

// Scan multiple rows
TEST_F(gaia_object_test, new_set_ins) {
    gaia_base_t::begin_transaction();
    get_field("Harold");
    get_field("Jameson");
    EXPECT_EQ(2, count_rows());
    gaia_base_t::commit_transaction();
}

// Read back from new, unsaved object
TEST_F(gaia_object_test, net_set_get) {
    gaia_base_t::begin_transaction();
    auto e = new Employee();
    e->set_name_last("Smith");
    EXPECT_STREQ(e->name_last(), "Smith");
    EXPECT_STREQ(e->name_last_original(), "Smith");
    // note: no row inserted
    delete e;
    gaia_base_t::commit_transaction();
}

// Attempt to read original value from new object
TEST_F(gaia_object_test, read_original_from_copy) {
    gaia_base_t::begin_transaction();
    auto e = get_field("Zachary");
    EXPECT_STREQ("Zachary", e->name_first_original());
    EXPECT_STREQ("Zachary", e->name_first());
    gaia_base_t::commit_transaction();
}

// Insert a row with no field values
TEST_F(gaia_object_test, new_ins_get) {
    gaia_base_t::begin_transaction();
    
    auto e = new Employee();
    e->insert_row();
    auto name = e->name_first();
    auto hire_date = e->hire_date();

    EXPECT_EQ(name, nullptr);
    EXPECT_EQ(0, hire_date);

    gaia_base_t::commit_transaction();
}

// Delete an un-inserted object with field values
TEST_F(gaia_object_test, new_get) {
    gaia_base_t::begin_transaction();
    
    auto e = new Employee();
    auto name = e->name_first();
    auto hire_date = e->hire_date();

    EXPECT_EQ(name, nullptr);
    EXPECT_EQ(0, hire_date);
    delete e;

    gaia_base_t::commit_transaction();
}

TEST_F(gaia_object_test, new_del_field_ref) {
    // create GAIA-64 scenario
    gaia_base_t::begin_transaction();

    auto e = new Employee();
    e->insert_row();
    e->delete_row();
    auto name = e->name_first();
    EXPECT_EQ(name, nullptr);
    auto hire_date = e->hire_date();
    EXPECT_EQ(hire_date, 0);
    e->set_name_last("Hendricks");

    gaia_base_t::commit_transaction();
}

TEST_F(gaia_object_test, new_del_original_field_ref) {
    // create GAIA-64 scenario
    gaia_base_t::begin_transaction();

    auto e = new Employee();
    e->insert_row();
    e->delete_row();
    auto name = e->name_first_original();
    EXPECT_EQ(name, nullptr);
    auto hire_date = e->hire_date_original();
    EXPECT_EQ(hire_date, 0);

    gaia_base_t::commit_transaction();
}

// Test on existing objects found by ID
// ====================================

// Create, write two rows, read back by ID and verify
TEST_F(gaia_object_test, read_back_id) {
    gaia_base_t::begin_transaction();
    auto eid = get_field("Howard")->gaia_id();
    auto eid2 = get_field("Henry")->gaia_id();
    gaia_base_t::commit_transaction();

    gaia_base_t::begin_transaction();
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
    gaia_base_t::commit_transaction();
    // Since e was deleted, this will not be cleaned up in TearDown
    delete e;
}

// Create, write two rows, read back by scan and verify
TEST_F(gaia_object_test, read_back_scan) {
    gaia_base_t::begin_transaction();
    auto eid = get_field("Howard")->gaia_id();
    auto eid2 = get_field("Henry")->gaia_id();
    gaia_base_t::commit_transaction();

    gaia_base_t::begin_transaction();
    auto e = Employee::get_first();
    EXPECT_EQ(eid, e->gaia_id());
    EXPECT_STREQ("Howard", e->name_first());
    EXPECT_STREQ("Howard", e->name_first_original());
    e = e->get_next();
    EXPECT_EQ(eid2, e->gaia_id());
    EXPECT_STREQ("Henry", e->name_first());
    EXPECT_STREQ("Henry", e->name_first_original());
    gaia_base_t::commit_transaction();
}

// Used twice, below
void UpdateReadBack(bool update_flag) {
    gaia_base_t::begin_transaction();
    get_field("Howard");
    get_field("Henry");
    gaia_base_t::commit_transaction();

    gaia_base_t::begin_transaction();
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
    gaia_base_t::commit_transaction();

    gaia_base_t::begin_transaction();
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
    gaia_base_t::commit_transaction();
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
    gaia_base_t::begin_transaction();
    auto e = get_field("Hector");
    e->delete_row();
    e->insert_row();
    e->set_name_first("Hudson");
    if (e != nullptr) {
        delete e;
    }
    gaia_base_t::commit_transaction();
}

// Delete a found object then update
TEST_F(gaia_object_test, new_del_upd) {
    gaia_base_t::begin_transaction();
    auto e = get_field("Hector");
    e->delete_row();
    e->update_row();
    gaia_base_t::commit_transaction();
    delete e;
}

// Delete a found object then insert after, it's good again
TEST_F(gaia_object_test, found_del_ins) {
    gaia_base_t::begin_transaction();
    
    auto e = get_field("Hector");
    e->delete_row();
    e->insert_row();
    auto eid = e->gaia_id();
    gaia_base_t::commit_transaction();

    gaia_base_t::begin_transaction();
    e = Employee::get_row_by_id(eid);
    e->set_name_first("Hudson");
    if (e != nullptr) {
        delete e;
    }
    gaia_base_t::commit_transaction();

}

// Delete an inserted object then set field after, it's good again
TEST_F(gaia_object_test, new_del_set) {
    gaia_base_t::begin_transaction();
    auto e = get_field("Hector");
    e->delete_row();
    e->set_name_first("Howard");
    if (e != nullptr) {
        delete e;
    }
    gaia_base_t::commit_transaction();

}

// Delete a found object then update
TEST_F(gaia_object_test, found_del_upd) {
    gaia_base_t::begin_transaction();
    auto e = get_field("Hector");
    auto eid = e->gaia_id();
    gaia_base_t::commit_transaction();

    gaia_base_t::begin_transaction();
    e = Employee::get_row_by_id(eid);
    e->delete_row();
    e->update_row();
    if (e != nullptr) {
        delete e;
    }
    gaia_base_t::commit_transaction();

}

// EXCEPTION conditions
// ====================

// Attempt to insert one row twice
TEST_F(gaia_object_test, insert_x2) {
    gaia_base_t::begin_transaction();
    auto e = get_field("Zachary");
    EXPECT_THROW(e->insert_row(), duplicate_id);
    gaia_base_t::commit_transaction();
}

// Create, then get by ID, insert
// Used twice, below
void InsertIdX2(bool insert_flag) {
    gaia_base_t::begin_transaction();
    auto eid = get_field("Zachary")->gaia_id();
    gaia_base_t::commit_transaction();

    gaia_base_t::begin_transaction();
    auto e = Employee::get_row_by_id(eid);
    if (insert_flag) {
        e->set_name_first("Zerubbabel");
    }
    EXPECT_THROW(e->insert_row(), duplicate_id);
    gaia_base_t::commit_transaction();
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
    gaia_base_t::begin_transaction();
    Employee* emp = new Employee();
    emp->set_name_first("Judith");
    EXPECT_THROW(emp->update_row(), invalid_node_id);
    if (emp != nullptr) {
        delete emp;
    }
    //Employee* emp2 = new Employee(0);
    //printf("%s\n", emp2->name_first());
    gaia_base_t::commit_transaction();
}

// Delete an un-inserted object
TEST_F(gaia_object_test, new_del) {
    gaia_base_t::begin_transaction();
    auto e = new Employee();
    EXPECT_THROW(e->delete_row(), invalid_node_id);
    if (e != nullptr) {
        delete e;
    }
    gaia_base_t::commit_transaction();
}

// Delete a row twice
TEST_F(gaia_object_test, new_del_del) {
    gaia_base_t::begin_transaction();
    auto e = get_field("Hugo");
    // the first delete succeeds
    e->delete_row();
    // second one fails
    EXPECT_THROW(e->delete_row(), invalid_node_id);
    gaia_base_t::commit_transaction();
    delete e;
}

// Perform get_next() without a preceeding get_first()
TEST_F(gaia_object_test, next_first) {
    gaia_base_t::begin_transaction();
    auto e1 = get_field("Harold");
    auto e2 = get_field("Howard");
    auto e3 = get_field("Hank");
    auto e_test = e2->get_next();
    EXPECT_TRUE(e_test == e1 || e_test == e3 || e_test == nullptr);
    gaia_base_t::commit_transaction();
}
