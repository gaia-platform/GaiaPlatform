/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"
#include "gaia_object.hpp"
#include "addr_book_generated.h"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;
using namespace AddrBook;

namespace AddrBook {
    static const gaia_type_t kEmployeeType = 4;
    static const gaia_type_t kPhoneType = 5;
    static const gaia_type_t kAddressType = 6;
};

struct Employee : public gaia_object_t<AddrBook::kEmployeeType, Employee, employee, employeeT>
{
    Employee(gaia_id_t id) : gaia_object_t(id) {}
    Employee() = default;
    const char* name_first() const { return get_str(name_first); }
    const char* name_last() const { return get_str(name_last); }
    const char* ssn() const { return get_str(ssn); }
    gaia_id_t hire_date() const { return get_current(hire_date); }
    const char*  email() const { return get_str(email); }
    const char*  web() const { return get_str(web); }

    const char* name_first_original() const { return get_str_original(name_first); }
    const char* name_last_original() const { return get_str_original(name_last); }
    const char* ssn_original() const { return get_str_original(ssn); }
    gaia_id_t hire_date_original() const { return get_original(hire_date); }
    const char*  email_original() const { return get_str_original(email); }
    const char*  web_original() const { return get_str_original(web); }

    void set_name_first(const char* s) { set(name_first, s); }
    void set_name_last(const char* s) { set(name_last, s); }
    void set_ssn(const char* s) { set(ssn, s); }
    void set_hire_date(gaia_id_t i) { set(hire_date, i); }
    void set_email(const char* s) { set(email, s); }
    void set_web(const char* s) { set(web, s); }
}; // Employee 

class GaiaObjectTest : public ::testing::Test {
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
        gaia_mem_base::reset_engine();
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
TEST_F(GaiaObjectTest, GetField) {
    gaia_base_t::begin_transaction();
    get_field("Harold");
    gaia_base_t::commit_transaction();
}

// Delete one row
TEST_F(GaiaObjectTest, GetFieldDelete) {
    gaia_base_t::begin_transaction();
    auto e = get_field("Jameson");
    e->delete_row();
    delete e;
    gaia_base_t::commit_transaction();
}

// Scan multiple rows
TEST_F(GaiaObjectTest, NewSetIns) {
    gaia_base_t::begin_transaction();
    get_field("Harold");
    get_field("Jameson");
    EXPECT_EQ(2, count_rows());
    gaia_base_t::commit_transaction();
}

// Read back from new, unsaved object
TEST_F(GaiaObjectTest, NewSetGet) {
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
TEST_F(GaiaObjectTest, ReadOriginalFromCopy) {
    bool exception = false;
    gaia_base_t::begin_transaction();
    auto e = get_field("Zachary");
    auto name = e->name_first_original();
    EXPECT_STREQ("Zachary", e->name_first());
    gaia_base_t::commit_transaction();
}

// Insert a row with no field values
TEST_F(GaiaObjectTest, NewInsGet) {
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
TEST_F(GaiaObjectTest, NewGet) {
    gaia_base_t::begin_transaction();
    
    auto e = new Employee();
    auto name = e->name_first();
    auto hire_date = e->hire_date();

    EXPECT_EQ(name, nullptr);
    EXPECT_EQ(0, hire_date);
    delete e;

    gaia_base_t::commit_transaction();
}

// Test on existing objects found by ID
// ====================================

// Create, write two rows, read back by ID and verify
TEST_F(GaiaObjectTest, ReadBackID) {
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
    gaia_base_t::commit_transaction();
}

// Create, write two rows, read back by scan and verify
TEST_F(GaiaObjectTest, ReadBackScan) {
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
    if (update_flag)
        e->update_row();
    e = e->get_next();
    e->set_name_first("Gerald");
    e->set_name_last("Glickman");
    if (update_flag)
        e->update_row();
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
TEST_F(GaiaObjectTest, UpdateReadBack) {
    UpdateReadBack(true);
}

// Create, write two rows, set fields, update, read, verify
TEST_F(GaiaObjectTest, NoUpdateReadBack) {
    UpdateReadBack(false);
}

// EXCEPTION conditions
// ====================

// Attempt to insert one row twice
TEST_F(GaiaObjectTest, InsertX2) {
    bool exception = false;
    gaia_base_t::begin_transaction();
    auto e = get_field("Zachary");
    try {
        e->insert_row();
    }
    catch (duplicate_id& e) {
        exception = true;
    }
    gaia_base_t::commit_transaction();

    EXPECT_EQ(true, exception) << "duplicate_id exception not thrown";
}

TEST_F(GaiaObjectTest, NoTx) {
    bool exception = false;
    Employee* emp = nullptr;
    try {
        emp = get_field("Harold");
    }
    catch (tx_not_open& e) {
        exception = true;
    }
    if (emp != nullptr)
        delete emp;

    EXPECT_EQ(true, exception) << "tx_not_open exception not thrown";
}

// Attempt to update an un-inserted object
TEST_F(GaiaObjectTest, NewUpd) {
    gaia_base_t::begin_transaction();
    bool exception = false;
    Employee* emp = new Employee();
    try {
        emp->set_name_first("Judith");
        emp->update_row();
    }
    catch (invalid_node_id& e) {
        exception = true;
    }
    if (emp != nullptr)
        delete emp;

    EXPECT_EQ(true, exception) << "invalid_node_id exception not thrown";

    gaia_base_t::commit_transaction();
}

// Delete an un-inserted object
TEST_F(GaiaObjectTest, NewDel) {
    gaia_base_t::begin_transaction();

    bool exception = false;
    auto e = new Employee();
    try {
        e->delete_row();
    }
    catch (invalid_node_id& e) {
        exception = true;
    }

    EXPECT_EQ(true, exception) << "invalid_node_id exception not thrown";
    delete e;

    gaia_base_t::commit_transaction();
}

