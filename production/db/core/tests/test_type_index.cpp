/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia_internal/common/mmap_helpers.hpp"
#include "gaia_internal/db/db_types.hpp"

#include "type_index.hpp"
#include "type_index_cursor.hpp"

using namespace gaia::common;
using namespace gaia::db;
class type_index_test : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Allocate memory for the type index.
        map_fd_data(
            m_type_index,
            sizeof(type_index_t),
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
            -1,
            0);
    }

    void TearDown() override
    {
        // Deallocate memory for the type index.
        unmap_fd_data(m_type_index, sizeof(type_index_t));
    }

protected:
    static constexpr gaia_type_t c_first_type{1};
    static constexpr gaia_locator_t c_first_locator{1};
    type_index_t* m_type_index{};
};

// Add one locator and verify that the cursor finds it.
TEST_F(type_index_test, add_and_find_one_locator)
{
    m_type_index->add_locator(c_first_type, c_first_locator);
    type_index_cursor_t cursor(m_type_index, c_first_type);
    EXPECT_EQ(cursor.current_locator(), c_first_locator);
    EXPECT_FALSE(cursor.next_locator().is_valid());
}

// Add and delete one locator and verify that the cursor finds it marked
// deleted.
TEST_F(type_index_test, add_and_delete_one_locator)
{
    m_type_index->add_locator(c_first_type, c_first_locator);
    EXPECT_TRUE(m_type_index->delete_locator(c_first_locator));
    type_index_cursor_t cursor(m_type_index, c_first_type);
    EXPECT_EQ(cursor.current_locator(), c_first_locator);
    EXPECT_TRUE(cursor.is_current_node_deleted());
    EXPECT_FALSE(cursor.next_locator().is_valid());
}

// Add and delete one locator, unlink it, and verify that the cursor doesn't
// find it on a new traversal.
TEST_F(type_index_test, add_delete_unlink_one_locator)
{
    m_type_index->add_locator(c_first_type, c_first_locator);
    EXPECT_TRUE(m_type_index->delete_locator(c_first_locator));
    type_index_cursor_t cursor(m_type_index, c_first_type);
    EXPECT_EQ(cursor.current_locator(), c_first_locator);
    EXPECT_TRUE(cursor.is_current_node_deleted());
    EXPECT_FALSE(cursor.next_locator().is_valid());
    EXPECT_TRUE(cursor.unlink_for_deletion());
    EXPECT_FALSE(cursor.current_locator().is_valid());
    cursor.reset();
    EXPECT_FALSE(cursor.current_locator().is_valid());
}

// Add multiple locators, delete and unlink the head (last locator added), and
// verify that it is not found and all other locators are found on a new
// traversal.
TEST_F(type_index_test, add_multi_delete_unlink_head_locator)
{
    constexpr size_t c_locator_count = 8;
    constexpr gaia_locator_t c_last_locator{c_locator_count};
    for (gaia_locator_t locator = c_first_locator; locator <= c_last_locator; ++locator)
    {
        m_type_index->add_locator(c_first_type, locator);
    }
    EXPECT_TRUE(m_type_index->delete_locator(c_last_locator));
    type_index_cursor_t cursor(m_type_index, c_first_type);
    EXPECT_EQ(cursor.current_locator(), c_last_locator);
    EXPECT_TRUE(cursor.is_current_node_deleted());
    EXPECT_TRUE(cursor.unlink_for_deletion());
    cursor.reset();
    size_t locators_found_count = 0;
    do
    {
        EXPECT_NE(cursor.current_locator(), c_last_locator);
        ++locators_found_count;
    } while (cursor.advance());
    EXPECT_EQ(locators_found_count, c_locator_count - 1);
}

// Add multiple locators, delete and unlink the tail (first locator added), and
// verify that it is not found and all other locators are found on a new
// traversal.
TEST_F(type_index_test, add_multi_delete_unlink_tail_locator)
{
    constexpr size_t c_locator_count = 8;
    constexpr gaia_locator_t c_last_locator{c_locator_count};
    for (gaia_locator_t locator = c_first_locator; locator <= c_last_locator; ++locator)
    {
        m_type_index->add_locator(c_first_type, locator);
    }
    EXPECT_TRUE(m_type_index->delete_locator(c_first_locator));
    type_index_cursor_t cursor(m_type_index, c_first_type);
    while (cursor.advance())
    {
        if (cursor.current_locator() == c_first_locator)
        {
            EXPECT_TRUE(cursor.is_current_node_deleted());
            EXPECT_TRUE(cursor.unlink_for_deletion());
        }
    }
    cursor.reset();
    size_t locators_found_count = 0;
    do
    {
        EXPECT_NE(cursor.current_locator(), c_first_locator);
        ++locators_found_count;
    } while (cursor.advance());
    EXPECT_EQ(locators_found_count, c_locator_count - 1);
}

// Add and delete multiple locators, unlink first one found (head), verify that
// no locators are found on a new traversal (since unlinking a deleted locator
// unlinks all consecutive deleted locators).
TEST_F(type_index_test, add_multi_delete_all_unlink_head)
{
    constexpr size_t c_locator_count = 8;
    constexpr gaia_locator_t c_last_locator{c_locator_count};
    for (gaia_locator_t locator = c_first_locator; locator <= c_last_locator; ++locator)
    {
        m_type_index->add_locator(c_first_type, locator);
    }
    for (gaia_locator_t locator = c_first_locator; locator <= c_last_locator; ++locator)
    {
        EXPECT_TRUE(m_type_index->delete_locator(locator));
    }
    type_index_cursor_t cursor(m_type_index, c_first_type);
    EXPECT_EQ(cursor.current_locator(), c_last_locator);
    EXPECT_TRUE(cursor.is_current_node_deleted());
    EXPECT_TRUE(cursor.unlink_for_deletion());
    cursor.reset();
    EXPECT_FALSE(cursor.current_locator().is_valid());
}

// Add multiple locators, delete all but first locator added (tail), unlink
// first locator found (head), verify that only first locator added (tail) is
// found on a new traversal (since unlinking a deleted locator unlinks all
// consecutive deleted locators).
TEST_F(type_index_test, add_multi_delete_all_except_head_unlink_all_deleted)
{
    constexpr size_t c_locator_count = 8;
    constexpr gaia_locator_t c_last_locator{c_locator_count};
    for (gaia_locator_t locator = c_first_locator; locator <= c_last_locator; ++locator)
    {
        m_type_index->add_locator(c_first_type, locator);
    }
    for (gaia_locator_t locator = c_first_locator + 1; locator <= c_last_locator; ++locator)
    {
        EXPECT_TRUE(m_type_index->delete_locator(locator));
    }
    type_index_cursor_t cursor(m_type_index, c_first_type);
    EXPECT_EQ(cursor.current_locator(), c_last_locator);
    EXPECT_TRUE(cursor.is_current_node_deleted());
    EXPECT_TRUE(cursor.unlink_for_deletion());
    cursor.reset();
    EXPECT_EQ(cursor.current_locator(), c_first_locator);
    EXPECT_FALSE(cursor.next_locator().is_valid());
}

// Add multiple locators, delete all but last locator added (head), unlink second
// locator found (successor of head), verify that only last locator added (head)
// is found on a new traversal (since unlinking a deleted locator unlinks all
// consecutive deleted locators).
TEST_F(type_index_test, add_multi_delete_all_except_tail_unlink_all_deleted)
{
    constexpr size_t c_locator_count = 8;
    constexpr gaia_locator_t c_last_locator{c_locator_count};
    for (gaia_locator_t locator = c_first_locator; locator <= c_last_locator; ++locator)
    {
        m_type_index->add_locator(c_first_type, locator);
    }
    for (gaia_locator_t locator = c_first_locator; locator < c_last_locator; ++locator)
    {
        EXPECT_TRUE(m_type_index->delete_locator(locator));
    }
    type_index_cursor_t cursor(m_type_index, c_first_type);
    EXPECT_EQ(cursor.current_locator(), c_last_locator);
    EXPECT_FALSE(cursor.is_current_node_deleted());
    EXPECT_TRUE(cursor.advance());
    EXPECT_TRUE(cursor.is_current_node_deleted());
    EXPECT_TRUE(cursor.unlink_for_deletion());
    cursor.reset();
    EXPECT_EQ(cursor.current_locator(), c_last_locator);
    EXPECT_FALSE(cursor.next_locator().is_valid());
}

// Add multiple locators, delete all but first and last locators added (head and
// tail), unlink second locator found (successor of head), verify that only
// first and last locators added (head and tail) are found on a new traversal
// (since unlinking a deleted locator unlinks all consecutive deleted locators).
TEST_F(type_index_test, add_multi_delete_all_except_head_and_tail_unlink_all_deleted)
{
    constexpr size_t c_locator_count = 8;
    constexpr gaia_locator_t c_last_locator{c_locator_count};
    for (gaia_locator_t locator = c_first_locator; locator <= c_last_locator; ++locator)
    {
        m_type_index->add_locator(c_first_type, locator);
    }
    for (gaia_locator_t locator = c_first_locator + 1; locator < c_last_locator; ++locator)
    {
        EXPECT_TRUE(m_type_index->delete_locator(locator));
    }
    type_index_cursor_t cursor(m_type_index, c_first_type);
    EXPECT_EQ(cursor.current_locator(), c_last_locator);
    EXPECT_FALSE(cursor.is_current_node_deleted());
    EXPECT_TRUE(cursor.advance());
    EXPECT_TRUE(cursor.is_current_node_deleted());
    EXPECT_TRUE(cursor.unlink_for_deletion());
    cursor.reset();
    EXPECT_EQ(cursor.current_locator(), c_last_locator);
    EXPECT_TRUE(cursor.advance());
    EXPECT_EQ(cursor.current_locator(), c_first_locator);
    EXPECT_FALSE(cursor.next_locator().is_valid());
}

// Add multiple locators, delete only first and last locators added (head and
// tail), unlink first and last locators found (head and tail), verify that all
// but first and last locators added (head and tail) are found on a new
// traversal.
TEST_F(type_index_test, add_multi_delete_only_head_and_tail_unlink_all_deleted)
{
    constexpr size_t c_locator_count = 8;
    constexpr gaia_locator_t c_last_locator{c_locator_count};
    for (gaia_locator_t locator = c_first_locator; locator <= c_last_locator; ++locator)
    {
        m_type_index->add_locator(c_first_type, locator);
    }
    EXPECT_TRUE(m_type_index->delete_locator(c_first_locator));
    EXPECT_TRUE(m_type_index->delete_locator(c_last_locator));
    type_index_cursor_t cursor(m_type_index, c_first_type);
    EXPECT_EQ(cursor.current_locator(), c_last_locator);
    EXPECT_TRUE(cursor.is_current_node_deleted());
    EXPECT_TRUE(cursor.unlink_for_deletion());
    while (cursor.next_locator().is_valid())
    {
        EXPECT_TRUE(cursor.advance());
    }
    EXPECT_EQ(cursor.current_locator(), c_first_locator);
    EXPECT_TRUE(cursor.is_current_node_deleted());
    EXPECT_TRUE(cursor.unlink_for_deletion());
    cursor.reset();
    size_t locators_found_count = 0;
    do
    {
        EXPECT_NE(cursor.current_locator(), c_first_locator);
        EXPECT_NE(cursor.current_locator(), c_last_locator);
        ++locators_found_count;
    } while (cursor.advance());
    EXPECT_EQ(locators_found_count, c_locator_count - 2);
}

// Try to exceed the registered type limit and verify the expected exception is
// thrown.
TEST_F(type_index_test, exceed_registered_type_limit)
{
    gaia_locator_t locator = c_first_locator;
    for (gaia_type_t type_id{c_first_type}; type_id.value() <= c_max_types; ++type_id, ++locator)
    {
        EXPECT_NO_THROW(m_type_index->add_locator(type_id, locator));
    }
    // Now try to add a locator for another type after we've already reached the type limit.
    EXPECT_THROW(m_type_index->add_locator(c_max_types + 1, locator + 1), type_limit_exceeded);
}

// For as many types as allowed by the type limit, add a locator to each list
// and verify its presence, delete the locator from each list and verify its
// logically deleted status, and finally unlink the locator from each list and
// verify its absence.
TEST_F(type_index_test, add_delete_unlink_locators_multiple_types)
{
    // For each type, add a locator to the index for that type and verify its
    // presence.
    gaia_locator_t locator = c_first_locator;
    for (gaia_type_t type_id{c_first_type}; type_id.value() <= c_max_types; ++type_id, ++locator)
    {
        m_type_index->add_locator(type_id, locator);
        type_index_cursor_t cursor(m_type_index, type_id);
        EXPECT_EQ(cursor.current_locator(), locator);
        EXPECT_FALSE(cursor.next_locator().is_valid());
    }
    // For each type, logically delete the locator in the index for
    // that type, verify its deleted status, then unlink it.
    locator = c_first_locator;
    for (gaia_type_t type_id{c_first_type}; type_id.value() <= c_max_types; ++type_id, ++locator)
    {
        EXPECT_TRUE(m_type_index->delete_locator(locator));
        type_index_cursor_t cursor(m_type_index, type_id);
        EXPECT_EQ(cursor.current_locator(), locator);
        EXPECT_FALSE(cursor.next_locator().is_valid());
        EXPECT_TRUE(cursor.is_current_node_deleted());
        EXPECT_TRUE(cursor.unlink_for_deletion());
    }
    // For each type, verify that there are no locators in the index for that
    // type.
    for (gaia_type_t type_id = c_first_type; type_id.value() <= c_max_types; ++type_id)
    {
        type_index_cursor_t cursor(m_type_index, type_id);
        EXPECT_FALSE(cursor.current_locator().is_valid());
        EXPECT_FALSE(cursor.advance());
    }
}
