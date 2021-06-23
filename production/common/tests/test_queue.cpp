/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

#include "gaia_internal/common/queue.hpp"

using namespace std;
using namespace gaia::common;

TEST(common, queue)
{
    queue_t<int> queue;

    int value = -1;

    // Verify that the queue is empty.
    queue.dequeue(value);
    ASSERT_EQ(-1, value);
    ASSERT_EQ(0, queue.size());

    // Insert two values.
    queue.enqueue(1);
    queue.enqueue(2);
    ASSERT_EQ(2, queue.size());

    // Extract a value.
    queue.dequeue(value);
    ASSERT_EQ(1, value);
    ASSERT_EQ(1, queue.size());

    // Insert another value.
    queue.enqueue(3);
    ASSERT_EQ(2, queue.size());

    // Extract the two values.
    queue.dequeue(value);
    ASSERT_EQ(2, value);
    ASSERT_EQ(1, queue.size());

    queue.dequeue(value);
    ASSERT_EQ(3, value);
    ASSERT_EQ(0, queue.size());

    // Verify that the queue is empty again.
    value = -1;
    queue.dequeue(value);
    ASSERT_EQ(-1, value);
    ASSERT_EQ(0, queue.size());

    // Insert one more value.
    queue.enqueue(4);
    ASSERT_EQ(1, queue.size());

    // Extract the value.
    queue.dequeue(value);
    ASSERT_EQ(4, value);
    ASSERT_EQ(0, queue.size());

    // Verify that the queue is empty again.
    value = 0;
    queue.dequeue(value);
    ASSERT_EQ(0, value);
    ASSERT_EQ(0, queue.size());
}

TEST(common, mpsc_queue)
{
    mpsc_queue_t<int> queue;

    int value = -1;

    // Verify that the queue is empty.
    queue.dequeue(value);
    ASSERT_EQ(-1, value);
    ASSERT_EQ(0, queue.size());

    // Insert two values.
    queue.enqueue(1);
    queue.enqueue(2);
    ASSERT_EQ(2, queue.size());

    // Extract a value.
    queue.dequeue(value);
    ASSERT_EQ(1, value);
    ASSERT_EQ(1, queue.size());

    // Insert another value.
    queue.enqueue(3);
    ASSERT_EQ(2, queue.size());

    // Extract the two values.
    queue.dequeue(value);
    ASSERT_EQ(2, value);
    ASSERT_EQ(1, queue.size());

    queue.dequeue(value);
    ASSERT_EQ(3, value);
    ASSERT_EQ(0, queue.size());

    // Verify that the queue is empty again.
    value = -1;
    queue.dequeue(value);
    ASSERT_EQ(-1, value);
    ASSERT_EQ(0, queue.size());

    // Insert one more value.
    queue.enqueue(4);
    ASSERT_EQ(1, queue.size());

    // Extract the value.
    queue.dequeue(value);
    ASSERT_EQ(4, value);
    ASSERT_EQ(0, queue.size());

    // Verify that the queue is empty again.
    value = 0;
    queue.dequeue(value);
    ASSERT_EQ(0, value);
    ASSERT_EQ(0, queue.size());
}
