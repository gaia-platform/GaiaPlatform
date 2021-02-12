/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

#include "db_internal_types.hpp"

using namespace gaia::db;

TEST(db_internals, mapped_log_t)
{
    mapped_log_t log1;
    mapped_log_t log2;

    ASSERT_FALSE(log1.is_initialized());
    ASSERT_FALSE(log2.is_initialized());

    log2.create("db_internals_log_test");

    ASSERT_FALSE(log1.is_initialized());
    ASSERT_TRUE(log2.is_initialized());

    log1.reset(log2);

    ASSERT_TRUE(log1.is_initialized());
    ASSERT_FALSE(log2.is_initialized());

    log1.close();

    ASSERT_FALSE(log1.is_initialized());
    ASSERT_FALSE(log2.is_initialized());
}
