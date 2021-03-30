/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"

#include <gaia_internal/common/pluralize.hpp>

using gaia::common::to_plural;

TEST(pluralize_test, pluralize)
{
    ASSERT_EQ("cats", to_plural("cat"));
    ASSERT_EQ("customers", to_plural("customer"));
    ASSERT_EQ("moneis", to_plural("money"));
    ASSERT_EQ("chesses", to_plural("chess"));
    ASSERT_EQ("cashes", to_plural("cash"));
}
