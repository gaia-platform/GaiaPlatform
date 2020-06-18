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
        begin_transaction();
        for(auto e = Employee::get_first();
            e;
            e = e->get_next())
        {
            e->delete_row();
        }
        commit_transaction();
        gaia_base_t::s_gaia_cache.clear();
        gaia_base_t::s_obj_cache.clear();
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

Employee_ptr set_field(const char* name) {
    auto e = Employee::create();
    e->set_name_first(name);
    return e;
}

Employee_ptr insert_row(const char* name) {
    auto e = set_field(name);
    EXPECT_NE(e, nullptr);
    e->insert_row();
    return e;
}

Employee_ptr update_row(const char* name) {
    auto e = insert_row(name);
    EXPECT_NE(e, nullptr);
    e->update_row();
    return e;
}

Employee_ptr get_field(const char* name) {
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
    auto e = Employee::create();
    e->set_name_last("Smith");
    EXPECT_STREQ(e->name_last(), "Smith");
    EXPECT_STREQ(e->name_last_original(), "Smith");
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
    
    auto e = Employee::create();
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
    
    auto e = Employee::create();
    auto name = e->name_first();
    auto hire_date = e->hire_date();

    EXPECT_EQ(name, nullptr);
    EXPECT_EQ(0, hire_date);
    commit_transaction();
}

TEST_F(gaia_object_test, new_del_field_ref) {
    // create GAIA-64 scenario
    begin_transaction();

    auto e = Employee::create();
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

    auto e = Employee::create();
    e->insert_row();
    e->delete_row();
    auto name = e->name_first_original();
    EXPECT_EQ(name, nullptr);
    auto hire_date = e->hire_date_original();
    EXPECT_EQ(hire_date, 0);

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
}

// Create row, try getting row from wrong type
TEST_F(gaia_object_test, read_wrong_type) {
    begin_transaction();
    auto e = get_field("Howard");
    auto eid = e->gaia_id();
    commit_transaction();

    begin_transaction();
    try {
        auto a = Address::get_row_by_id(eid);
        gaia_id_t id = a->gaia_id();
        EXPECT_EQ(id, 1000);
    }
    catch (const exception& e) {
        // The eid is unpredictable, but the exception will use it in its message.
        string compare_string = "requesting Gaia type Address(2) but object identified by " + to_string(eid) + " is type Employee(1)";
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
    commit_transaction();
}

// Delete a found object then update
TEST_F(gaia_object_test, new_del_upd) {
    begin_transaction();
    auto e = get_field("Hector");
    e->delete_row();
    e->update_row();
    commit_transaction();
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
    commit_transaction();
}

// Delete an inserted object then set field after, it's good again
TEST_F(gaia_object_test, new_del_set) {
    begin_transaction();
    auto e = get_field("Hector");
    e->delete_row();
    e->set_name_first("Howard");
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
    auto emp = Employee::create();
    emp->set_name_first("Judith");
    EXPECT_THROW(emp->update_row(), invalid_node_id);
    //Employee* emp2 = new Employee(0);
    //printf("%s\n", emp2->name_first());
    commit_transaction();
}

// Delete an un-inserted object
TEST_F(gaia_object_test, new_del) {
    begin_transaction();
    auto e = Employee::create();
    EXPECT_THROW(e->delete_row(), invalid_node_id);
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
}

// Perform get_next() without a preceeding get_first()
TEST_F(gaia_object_test, next_first) {
    begin_transaction();
    auto e1 = get_field("Harold");
    auto e2 = get_field("Howard");
    auto e3 = get_field("Hank");
    auto e_test = e2->get_next();
    EXPECT_TRUE(e_test == e1 || e_test == e3 || e_test == nullptr);
    auto e4 = Employee::create();
    // In this case, the row doesn't exist yet.
    e4->set_name_first("Hector");
    EXPECT_EQ(nullptr, e4->get_next());
    commit_transaction();
}

// Helpers for "simplified" API sanity tests
void verify_cache_counts(uint32_t expected_gaia_cache_size, uint32_t expected_obj_cache_size)
{
    EXPECT_EQ(gaia_base_t::s_gaia_cache.size(), expected_gaia_cache_size);
    EXPECT_EQ(gaia_base_t::s_obj_cache.size(), expected_obj_cache_size);
}

void do_commit_begin(auto_transaction_t * tx)
{
    if (tx)
    {
        tx->commit();
    }
    else
    {
        commit_transaction();
        begin_transaction();
    }
}

// If tx is non-null then test implicit and auto-begin
// transactions.  Otherwise, use the same model
// The two should have the exact same semantics.
void test_api(auto_transaction_t * tx) {
    gaia_id_t id = 0;

    // insert
    if (!tx) {
        begin_transaction();
    }
    auto e = Employee::create();
    verify_cache_counts(0, 1);
    e->set_name_last("Hawkins");
    if (!tx) { 
        e->insert_row();
    }
    do_commit_begin(tx);

    EXPECT_STREQ(e->name_last(), "Hawkins");
    verify_cache_counts(1, 0);
    id = e->gaia_id();

    // auto update
    e->set_name_last("Clinton");
    if (!tx) { 
        e->update_row();
    }
    do_commit_begin(tx);
    verify_cache_counts(1, 0);
    EXPECT_STREQ(e->name_last(), "Clinton");

    // Object went out of scope so no insert
    // in the non-simplified this isn't an issue
    // because you have no object off of which to
    // call insert_row().
    {
        auto a = Address::create();
        verify_cache_counts(1, 1);
        a->set_city("Seattle");
    }
    verify_cache_counts(1, 1);
    do_commit_begin(tx);
    auto address = Address::get_first();
    EXPECT_TRUE(!address);
    verify_cache_counts(1, 0);

    // Update for an object that goes out of scope
    // should also not work.  Again not an issue
    // in the non-simplified API because there is
    // no object off of which youc an call update_row()
    {
        auto a = Address::create();
        verify_cache_counts(1, 1);
        a->set_city("Seattle");
        if (!tx) { 
            a->insert_row();
        }
        do_commit_begin(tx);
        id = a->gaia_id();
        a->set_city("Portland");
        verify_cache_counts(2, 0);
    }
    do_commit_begin(tx);
    auto a = Address::get_row_by_id(id);
    EXPECT_STREQ(a->city(), "Seattle");
}

// Verify our low_api behavor:
// 1) auto_transaction will begin a transaction
//   immediately after commit
// 2) explict insert_row() call not needed
// 3) explicit update_row() call not needed
TEST_F(gaia_object_test, simplified_api) {
    auto_transaction_t tx;
    test_api(&tx);
}

TEST_F(gaia_object_test, standard_api) {
    test_api(nullptr);
}

TEST_F(gaia_object_test, simplified_api_example) {
    /**
     * Below code is what the simplified example is doing
     */

    /*
    begin_transaction();
    Employee_ptr e = Employee::create();
    e->set_name_last("Smith");
    e->insert_row();
    commit_transaction();

    begin_transaction();
    EXPECT_STREQ(e->name_last(), "Smith");
    e->set_name_last("Doe");
    e->update_row();
    commit_transaction();

    begin_transaction();
    EXPECT_STREQ(e->name_last(), "Doe");
    commit_transaction();
    */

    auto_transaction_t tx;
    Employee_ptr e = Employee::create();
    e->set_name_last("Smith");
    tx.commit();
    
    EXPECT_STREQ(e->name_last(), "Smith");
    e->set_name_last("Doe");
    e->update_row();
    tx.commit();
    
    EXPECT_STREQ(e->name_last(), "Doe");
    tx.commit(false);
}
