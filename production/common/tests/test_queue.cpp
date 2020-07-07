/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

#include <queue.hpp>

using namespace std;
using namespace gaia::common;

TEST(common, queue)
{
    queue_t<int> q;

    int value = -1;

    // Verify that the queue is empty.
    q.dequeue(value);
    ASSERT_EQ(-1, value);

    // Insert two values.
    q.enqueue(1);
    q.enqueue(2);

    // Extract a value.
    q.dequeue(value);
    ASSERT_EQ(1, value);

    // Insert another value.
    q.enqueue(3);

    // Extract the two values.
    q.dequeue(value);
    ASSERT_EQ(2, value);

    q.dequeue(value);
    ASSERT_EQ(3, value);

    // Verify that the queue is empty again.
    value = -1;
    q.dequeue(value);
    ASSERT_EQ(-1, value);

    // Insert one more value.
    q.enqueue(4);

    // Extract the value.
    q.dequeue(value);
    ASSERT_EQ(4, value);

    // Verify that the queue is empty again.
    value = 0;
    q.dequeue(value);
    ASSERT_EQ(0, value);
}
