/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

#include "queue.hpp"

using namespace std;
using namespace gaia::common;

TEST(common, queue)
{
    queue_t<int> queue;

    int value = -1;

    // Verify that the queue is empty.
    queue.dequeue(value);
    ASSERT_EQ(-1, value);

    // Insert two values.
    queue.enqueue(1);
    queue.enqueue(2);

    // Extract a value.
    queue.dequeue(value);
    ASSERT_EQ(1, value);

    // Insert another value.
    queue.enqueue(3);

    // Extract the two values.
    queue.dequeue(value);
    ASSERT_EQ(2, value);

    queue.dequeue(value);
    ASSERT_EQ(3, value);

    // Verify that the queue is empty again.
    value = -1;
    queue.dequeue(value);
    ASSERT_EQ(-1, value);

    // Insert one more value.
    queue.enqueue(4);

    // Extract the value.
    queue.dequeue(value);
    ASSERT_EQ(4, value);

    // Verify that the queue is empty again.
    value = 0;
    queue.dequeue(value);
    ASSERT_EQ(0, value);
}
