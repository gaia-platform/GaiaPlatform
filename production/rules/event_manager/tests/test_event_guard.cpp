/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include "gtest/gtest.h"
#include "event_guard.hpp"

using namespace std;
using namespace gaia::rules;
using namespace gaia::common;

// Guardee refers to the state of the logged events before they enter the guard.
// Guarder refers to the event_guard class under test that actually sets the guard bits
// and determines whether it should block the log_event call or not.
void verify_guard(database_event_guard_t& guardee, event_guard_t& guarder, gaia_type_t gaia_type, bool expected_blocked, 
    uint32_t expected_bitmap)
{
    uint32_t bitmap = guardee[gaia_type];
    EXPECT_EQ(expected_blocked, guarder.is_blocked());
    EXPECT_EQ(expected_bitmap, bitmap);
}

void verify_post_guard(database_event_guard_t& guardee, gaia_type_t gaia_type, uint32_t expected_bitmap)
{
    uint32_t bitmap = guardee[gaia_type];
    EXPECT_EQ(expected_bitmap, bitmap);
}

void verify_guard(field_event_guard_t& guardee, event_guard_t& guarder, 
    gaia_type_t gaia_type, const char* field, bool expected_blocked, 
    uint32_t expected_bitmap)
{
    uint32_t bitmap = guardee[gaia_type][field];
    EXPECT_EQ(expected_blocked, guarder.is_blocked());
    EXPECT_EQ(expected_bitmap, bitmap);
}

void verify_post_guard(field_event_guard_t& guardee, gaia_type_t gaia_type, const char* field, uint32_t expected_bitmap)
{
    uint32_t bitmap = guardee[gaia_type][field];
    EXPECT_EQ(expected_bitmap, bitmap);
}

TEST(event_guard_test, database_event_guard_no_block)
{
    database_event_guard_t database_event_guard;
    event_type_t insert = event_type_t::row_insert;
    {
        // Insert event is running; bitmap should be set.
        event_guard_t guard(database_event_guard, 333, insert);
        verify_guard(database_event_guard, guard, 333, false, (uint32_t)insert);
    }
    // Insert event is done running so bitmap should be cleared.
    verify_post_guard(database_event_guard, 333, 0);
}

TEST(event_guard_test, database_event_guard_block)
{
    database_event_guard_t database_event_guard;
    event_type_t insert = event_type_t::row_insert;
    {
        // Insert event is running; bitmap should be set.
        event_guard_t guard(database_event_guard, 333, insert);
        verify_guard(database_event_guard, guard, 333, false, (uint32_t)insert);
        {
            event_guard_t guard(database_event_guard, 333, insert);
            verify_guard(database_event_guard, guard, 333, true, (uint32_t)insert);
        }
        // Insert event is still running so bitmap should not be cleared
        verify_post_guard(database_event_guard, 333, (uint32_t)insert);

    }
    // Insert event is done running so bitmap should be cleared.
    verify_post_guard(database_event_guard, 333, 0);
}

TEST(event_guard_test, database_event_guard_no_block_type)
{
    database_event_guard_t database_event_guard;
    event_type_t insert = event_type_t::row_insert;
    {
        // Insert event for type 333 is running; bitmap should be set.
        event_guard_t guard(database_event_guard, 333, insert);
        verify_guard(database_event_guard, guard, 333, false, (uint32_t)insert);
        {
            // Insert event for type 444 is running; bitmap should be set.
            event_guard_t guard(database_event_guard, 444, insert);
            verify_guard(database_event_guard, guard, 444, false, (uint32_t)insert);
        }
        // Insert event for type 444 has been cleared
        verify_post_guard(database_event_guard, 444, 0);
        // Insert event for type 333 is still running
        verify_post_guard(database_event_guard, 333, (uint32_t)insert);

    }
    // Insert event is done running so bitmap should be cleared.
    verify_post_guard(database_event_guard, 333, 0);
}

TEST(event_guard_test, database_event_guard_no_block_event)
{
    database_event_guard_t database_event_guard;
    event_type_t insert = event_type_t::row_insert;
    {
        // Insert event is running; insert bit should be set.
        event_guard_t guard(database_event_guard, 333, insert);
        verify_guard(database_event_guard, guard, 333, false, (uint32_t)insert);
        {
            event_type_t deleted = event_type_t::row_delete;
            // Delete event is running; bitmap should now track both delete and insert events.
            event_guard_t guard(database_event_guard, 333, deleted);
            verify_guard(database_event_guard, guard, 333, false, (uint32_t) insert | (uint32_t)deleted);
        }
        // Insert event for type 333 is still running but delete bit should 
        // have been cleared.
        verify_post_guard(database_event_guard, 333, (uint32_t)insert);
    }
    // Insert event is done running so bitmap should be cleared.
    verify_post_guard(database_event_guard, 333, 0);
}

TEST(event_guard_test, field_event_guard_no_block)
{
    field_event_guard_t field_event_guard;
    event_type_t write = event_type_t::field_write;
    {
        // Write event is running for "last_name" field; bitmap should be set.
        event_guard_t guard(field_event_guard, 333, "last_name", write);
        verify_guard(field_event_guard, guard, 333, "last_name", false, (uint32_t) write);
    }
    // Write event is done running so bitmap should be cleared.
    verify_post_guard(field_event_guard, 333, "last_name", 0);
}

TEST(event_guard_test, field_event_guard_block)
{
    field_event_guard_t field_event_guard;
    event_type_t write = event_type_t::field_write;
    {
        // Write event is running for "last_name" field; bitmap should be set.
        event_guard_t guard(field_event_guard, 333, "last_name", write);
        verify_guard(field_event_guard, guard, 333, "last_name", false, (uint32_t)write);
        {
            event_guard_t guard(field_event_guard, 333, "last_name", write);
            verify_guard(field_event_guard, guard, 333, "last_name", true, (uint32_t)write);
        }
        // Insert event is still running so bitmap should not be cleared
        verify_post_guard(field_event_guard, 333, "last_name", (uint32_t)write);

    }
    // Insert event is done running so bitmap should be cleared.
    verify_post_guard(field_event_guard, 333, "last_name", 0);
}

TEST(event_guard_test, field_event_guard_no_block_type)
{
    field_event_guard_t field_event_guard;
    event_type_t write = event_type_t::field_write;
    {
        // Write event is running for "last_name" field; bitmap should be set.
        event_guard_t guard(field_event_guard, 333, "last_name", write);
        verify_guard(field_event_guard, guard, 333, "last_name", false, (uint32_t) write);
        {
            // Write event for type 444 is running for "last_name" field; bitmap should be set.
            event_guard_t guard(field_event_guard, 444, "last_name", write);
            verify_guard(field_event_guard, guard, 444, "last_name", false, (uint32_t) write);            
        }
        // Write event for type 444 has been cleared
        verify_post_guard(field_event_guard, 444, "last_name", 0);
        // Write event for type 333 is still running
        verify_post_guard(field_event_guard, 333, "last_name", (uint32_t)write);

    }
    // Write event is done running so bitmap should be cleared.
    verify_post_guard(field_event_guard, 333, "last_name", 0);
}

TEST(event_guard_test, field_event_guard_no_block_field)
{
    field_event_guard_t field_event_guard;
    event_type_t write = event_type_t::field_write;
    {
        // Write event is running for "last_name" field; bitmap should be set.
        event_guard_t guard(field_event_guard, 333, "last_name", write);
        verify_guard(field_event_guard, guard, 333, "last_name", false, (uint32_t) write);
        {
            // Write event for type 333 is running for "last_name" field; bitmap should be set.
            // Ensure we can write to a different field of the same gaia_type.  Now both
            // "last_name" and "first_name" fields should be set
            event_guard_t guard(field_event_guard, 333, "first_name", write);
            verify_guard(field_event_guard, guard, 333, "first_name", false, (uint32_t) write);
            verify_guard(field_event_guard, guard, 333, "last_name", false, (uint32_t) write);
        }
        // Write event for type 333 and column "first_name" should be cleared
        verify_post_guard(field_event_guard, 333, "first_name", 0);
        // Write event for type 333 and column "last_name" should still be set
        verify_post_guard(field_event_guard, 333, "last_name", (uint32_t)write);

    }
    // Write event is done running so bitmap should be cleared.
    verify_post_guard(field_event_guard, 333, "last_name", 0);
}


TEST(event_guard_test, field_event_guard_no_block_event)
{
    field_event_guard_t field_event_guard;
    event_type_t write = event_type_t::field_write;
    {
        // Write event is running for "last_name" field; bitmap should be set.
        event_guard_t guard(field_event_guard, 333, "last_name", write);
        verify_guard(field_event_guard, guard, 333, "last_name", false, (uint32_t) write);
        {
            event_type_t read = event_type_t::field_read;
            // Read event is running; bitmap should now track both read and write events
            event_guard_t guard(field_event_guard, 333, "last_name", read);
            verify_guard(field_event_guard, guard, 333, "last_name", false, (uint32_t)read | (uint32_t)write);
        }
        // Read event for type 333 should be cleared.  But write event is stil set.
        verify_post_guard(field_event_guard, 333, "last_name", (uint32_t)write);
    }
    // Insert event is done running so bitmap should be cleared.
    verify_post_guard(field_event_guard, 333, "last_name", 0);
}

TEST(event_guard_test, mixed_event_guard)
{
    field_event_guard_t field_event_guard;
    database_event_guard_t database_event_guard;
    event_type_t insert = event_type_t::row_insert;
    event_type_t write = event_type_t::field_write;
    event_type_t begin = event_type_t::transaction_begin;
    {
        // Start a begin transaction.
        event_guard_t guard(database_event_guard, 0, begin);
        verify_guard(database_event_guard, guard, 0, false, (uint32_t) begin);
        {
            // Start an insert.
            event_guard_t guard(database_event_guard, 333, insert);
            verify_guard(database_event_guard, guard, 0, false, (uint32_t) begin);
            verify_guard(database_event_guard, guard, 333, false, (uint32_t) insert);
            {
                // Now do a field write.
                event_guard_t guard(field_event_guard, 333, "last_name", write);
                verify_guard(field_event_guard, guard, 333, "last_name", false, (uint32_t) write);
                verify_post_guard(database_event_guard, 0, (uint32_t) begin);
                verify_post_guard(database_event_guard, 333, (uint32_t) insert);
            }
            // Write event cleared.
            verify_post_guard(field_event_guard, 333, "last_name", 0);
            // Begin and Insert events still active.
            verify_post_guard(database_event_guard, 0, (uint32_t) begin);
            verify_post_guard(database_event_guard, 333, (uint32_t) insert);
        }
        // Insert event cleared.
        verify_post_guard(field_event_guard, 333, "last_name", 0);
        verify_post_guard(database_event_guard, 333, 0);
        verify_post_guard(database_event_guard, 0, (uint32_t)begin);
    }
    // Begin event cleared;
    verify_post_guard(field_event_guard, 333, "last_name", 0);
    verify_post_guard(database_event_guard, 333, 0);
    verify_post_guard(database_event_guard, 0, 0);
}
