/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia/exceptions.hpp"

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "data_holder.hpp"
#include "gaia_prerequisites.h"
#include "hash_index.hpp"
#include "range_index.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::payload_types;
using namespace gaia::db::index;
using namespace gaia::direct_access;
using namespace gaia::prerequisites;

// Placeholder values for index records.
constexpr gaia::db::gaia_txn_id_t c_fake_txn_id = 777;
constexpr gaia::db::gaia_offset_t c_fake_offset = 0;

index_record_t create_index_record()
{
    thread_local static gaia::db::gaia_locator_t locator = 0;

    return {c_fake_txn_id, locator++, c_fake_offset, index_record_operation_t::insert};
}

TEST(index, empty_range_index)
{
    range_index_t range_index(0);
    size_t count = 0;

    for (auto it = range_index.begin(); it != range_index.end(); it++)
    {
        count++;
    }

    ASSERT_EQ(count, 0);
}

TEST(index, empty_hash_index)
{
    hash_index_t hash_index(0);
    size_t count = 0;
    for (auto it = hash_index.begin(); it != hash_index.end(); it++)
    {
        count++;
    }
    ASSERT_EQ(count, 0);
}

TEST(index, one_record_hash_index)
{
    hash_index_t hash_index(0);
    size_t count = 0;

    hash_index.insert_index_entry(index_key_t(0), index_record_t(create_index_record()));

    for (const auto& [k, record] : hash_index)
    {
        ASSERT_EQ(record.txn_id, c_fake_txn_id);
        count++;
    }

    ASSERT_EQ(count, 1);
}

TEST(index, one_record_range_index)
{
    range_index_t range_index(0);
    size_t count = 0;

    range_index.insert_index_entry(index_key_t(0), index_record_t(create_index_record()));

    for (const auto& [k, record] : range_index)
    {
        ASSERT_EQ(record.txn_id, c_fake_txn_id);
        count++;
    }

    ASSERT_EQ(count, 1);
}

TEST(index, single_key_multi_record_hash_index)
{
    hash_index_t hash_index(0);
    size_t count = 0;

    hash_index.insert_index_entry(index_key_t(0), index_record_t(create_index_record()));
    hash_index.insert_index_entry(index_key_t(0), index_record_t(create_index_record()));
    hash_index.insert_index_entry(index_key_t(0), index_record_t(create_index_record()));

    for (const auto& [k, record] : hash_index)
    {
        ASSERT_EQ(record.txn_id, c_fake_txn_id);
        count++;
    }

    ASSERT_EQ(count, 3);
}

TEST(index, single_key_multi_record_range_index)
{
    range_index_t range_index(0);
    size_t count = 0;

    range_index.insert_index_entry(index_key_t(0), index_record_t(create_index_record()));
    range_index.insert_index_entry(index_key_t(0), index_record_t(create_index_record()));
    range_index.insert_index_entry(index_key_t(0), index_record_t(create_index_record()));

    for (const auto& [k, record] : range_index)
    {
        ASSERT_EQ(record.txn_id, c_fake_txn_id);
        count++;
    }

    ASSERT_EQ(count, 3);
}

TEST(index, multi_key_multi_record_hash_index)
{
    hash_index_t hash_index(0);
    size_t count = 0;

    hash_index.insert_index_entry(index_key_t(0), index_record_t(create_index_record()));
    hash_index.insert_index_entry(index_key_t(1), index_record_t(create_index_record()));
    hash_index.insert_index_entry(index_key_t(2), index_record_t(create_index_record()));
    hash_index.insert_index_entry(index_key_t(2), index_record_t(create_index_record()));
    hash_index.insert_index_entry(index_key_t(3), index_record_t(create_index_record()));

    for (const auto& [k, record] : hash_index)
    {
        ASSERT_EQ(record.txn_id, c_fake_txn_id);
        count++;
    }

    ASSERT_EQ(count, 5);
}

TEST(index, multi_key_multi_record_range_index)
{
    range_index_t range_index(0);
    size_t count = 0;

    range_index.insert_index_entry(index_key_t(0), index_record_t(create_index_record()));
    range_index.insert_index_entry(index_key_t(1), index_record_t(create_index_record()));
    range_index.insert_index_entry(index_key_t(2), index_record_t(create_index_record()));
    range_index.insert_index_entry(index_key_t(2), index_record_t(create_index_record()));
    range_index.insert_index_entry(index_key_t(3), index_record_t(create_index_record()));

    for (const auto& [k, record] : range_index)
    {
        ASSERT_EQ(record.txn_id, c_fake_txn_id);
        count++;
    }

    ASSERT_EQ(count, 5);
}

// Simulate index updates for range index
TEST(index, range_update_test)
{
    index_record_t to_update = create_index_record();
    range_index_t range_index(0);
    size_t count = 0;

    range_index.insert_index_entry(index_key_t(0), to_update);
    range_index.insert_index_entry(index_key_t(1), to_update);
    to_update.operation = index_record_operation_t::remove;
    range_index.insert_index_entry(index_key_t(1), to_update);
    range_index.insert_index_entry(index_key_t(2), to_update);

    for (const auto& [k, record] : range_index)
    {
        ASSERT_EQ(record.txn_id, c_fake_txn_id);
        count++;
    }

    ASSERT_EQ(count, 3);
}

// Simulate index updates for hash index
TEST(index, hash_update_test)
{
    index_record_t to_update = create_index_record();
    hash_index_t hash_index(0);
    size_t count = 0;

    hash_index.insert_index_entry(index_key_t(0), to_update);
    hash_index.insert_index_entry(index_key_t(1), to_update);
    to_update.operation = index_record_operation_t::remove;
    hash_index.insert_index_entry(index_key_t(1), to_update);
    hash_index.insert_index_entry(index_key_t(2), to_update);

    for (const auto& [k, record] : hash_index)
    {
        ASSERT_EQ(record.txn_id, c_fake_txn_id);
        count++;
    }

    ASSERT_EQ(count, 3);
}

TEST(index, eq_range_hash_index)
{
    hash_index_t hash_index(0);
    size_t count = 0;

    hash_index.insert_index_entry(index_key_t(0), index_record_t(create_index_record()));
    hash_index.insert_index_entry(index_key_t(1), index_record_t(create_index_record()));
    hash_index.insert_index_entry(index_key_t(1), index_record_t(create_index_record()));
    hash_index.insert_index_entry(index_key_t(1), index_record_t(create_index_record()));
    hash_index.insert_index_entry(index_key_t(2), index_record_t(create_index_record()));
    hash_index.insert_index_entry(index_key_t(2), index_record_t(create_index_record()));

    auto range = hash_index.equal_range(1);
    for (auto it = range.first; it != range.second; ++it)
    {
        ASSERT_EQ(it->first, 1);
        count++;
    }

    ASSERT_EQ(count, 3);
}

TEST(index, eq_range_range_index)
{
    range_index_t range_index(0);
    size_t count = 0;

    range_index.insert_index_entry(index_key_t(0), index_record_t(create_index_record()));
    range_index.insert_index_entry(index_key_t(1), index_record_t(create_index_record()));
    range_index.insert_index_entry(index_key_t(1), index_record_t(create_index_record()));
    range_index.insert_index_entry(index_key_t(1), index_record_t(create_index_record()));
    range_index.insert_index_entry(index_key_t(2), index_record_t(create_index_record()));
    range_index.insert_index_entry(index_key_t(2), index_record_t(create_index_record()));

    auto range = range_index.equal_range(1);
    for (auto it = range.first; it != range.second; ++it)
    {
        ASSERT_EQ(it->first, 1);
        count++;
    }

    ASSERT_EQ(count, 3);
}

TEST(index, key_hash_test)
{
    // Check compound keys with different orders are distributed differently
    index_key_t k1(1, 0);
    index_key_t k2(0, 1);

    ASSERT_NE(index_key_hash{}(k1), index_key_hash{}(k2));

    // Check zero and empty keys are distributed differently
    index_key_t empty_key;
    index_key_t zero_key(0);
    index_key_t double_zero_key(0, 0);

    ASSERT_NE(index_key_hash{}(empty_key), index_key_hash{}(zero_key));
    ASSERT_NE(index_key_hash{}(zero_key), index_key_hash{}(double_zero_key));
}

class index_test : public db_catalog_test_base_t
{
protected:
    index_test()
        : db_catalog_test_base_t("prerequisites.ddl"){};
};

TEST_F(index_test, unique_constraint_same_txn)
{
    const char* student_id = "00002217";

    auto_transaction_t txn;
    student_t::insert_row(student_id, "Alice", 21, 30, 3.75);
    student_t::insert_row(student_id, "Bob", 22, 28, 3.5);
    ASSERT_THROW(txn.commit(), unique_constraint_exception);
}

TEST_F(index_test, unique_constraint_different_txn)
{
    const char* student_id = "00002217";

    auto_transaction_t txn;
    student_t::insert_row(student_id, "Alice", 21, 30, 3.75);
    txn.commit();

    // Attempt to re-insert the same key - we should trigger the conflict.
    student_t::insert_row(student_id, "Bob", 22, 28, 3.5);
    ASSERT_THROW(txn.commit(), unique_constraint_exception);
}

TEST_F(index_test, unique_constraint_update_record)
{
    const char* student_id = "00002217";

    auto_transaction_t txn;
    gaia_id_t id = student_t::insert_row(student_id, "Alice", 21, 30, 3.75);
    txn.commit();

    // Update the record - this should not trigger any conflict.
    student_t alice = student_t::get(id);
    auto alice_w = alice.writer();
    alice_w.surname = "Alicia";
    alice_w.update_row();
    txn.commit();

    // Verify the update.
    alice = student_t::get(id);
    ASSERT_EQ(0, strcmp(alice.surname(), "Alicia"));
}

TEST_F(index_test, unique_constraint_delete_record)
{
    const char* student_id = "00002217";

    auto_transaction_t txn;
    gaia_id_t id = student_t::insert_row(student_id, "Alice", 21, 30, 3.75);
    txn.commit();

    // Delete the record and reinsert the key - this should not trigger any conflict.
    student_t alice = student_t::get(id);
    alice.delete_row();
    id = student_t::insert_row(student_id, "Alicia", 21, 30, 3.75);
    txn.commit();

    // Verify the insert.
    alice = student_t::get(id);
    ASSERT_EQ(0, strcmp(alice.surname(), "Alicia"));
}

TEST_F(index_test, unique_constraint_rollback_transaction)
{
    const char* alice_student_id = "00002217";
    const char* bob_student_id = "00002346";

    auto_transaction_t txn;
    student_t::insert_row(alice_student_id, "Alice", 21, 30, 3.75);
    txn.commit();

    // Insert a second key and then attempt to re-insert the first key.
    // We should trigger the conflict and our transaction should be rolled back.
    student_t::insert_row(bob_student_id, "Bob", 22, 28, 3.5);
    student_t::insert_row(alice_student_id, "Charles", 22, 24, 3.25);
    ASSERT_THROW(txn.commit(), unique_constraint_exception);

    // Attempt to insert the second key again.
    // We should succeed if the previous transaction was properly rolled back.
    // We need to manually start a transaction because the exception generated earlier
    // prevented the automatic restart.
    txn.begin();
    student_t::insert_row(bob_student_id, "Bob", 22, 28, 3.5);
    txn.commit();
}
