/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include any internal headers to ensure that
// the code included in this file doesn't have a dependency
// on non-public APIs.

#include "test_sdk_base.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::rules;
using namespace gaia::addr_book;
using namespace gaia::db::triggers;

extern "C" void initialize_rules()
{
    // Verify this initialize_rules() is never called
    // as the db only shared library should never call
    // initialize_rules
    g_initialize_rules_called = true;
}

class sdk_db_no_init_rules__test : public sdk_base
{
};

TEST_F(sdk_db_no_init_rules__test, rules_not_initializEd)
{
    EXPECT_FALSE(g_initialize_rules_called);
}
