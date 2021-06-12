/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"

#include "data_holder.hpp"
#include "hash_index.hpp"
#include "range_index.hpp"

using namespace gaia::db::payload_types;
using namespace gaia::db::index;

// Placeholder values for index records.
constexpr gaia::db::gaia_txn_id_t c_fake_txn_id{777};
constexpr gaia::db::gaia_offset_t c_fake_offset{0};

index_record_t create_index_record()
{
    thread_local static auto locator = to_integral(gaia::db::c_invalid_gaia_locator);

    return {c_fake_txn_id, c_fake_offset, gaia::db::gaia_locator_t{locator++}, false};
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
    to_update.deleted = true;
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
    to_update.deleted = true;
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
