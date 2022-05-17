/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

// Don't provide a definition of `initialize_rules` to ensure we don't
// get a link error.  The other test (test_sdk.cpp) tests that a strong
// reference will override the weak reference and take precedence.

#include "test_sdk_base.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::addr_book;
using namespace gaia::db::triggers;

class sdk_no_init_rules__test : public sdk_base
{
};

TEST_F(sdk_no_init_rules__test, app_check)
{
    auto_transaction_t tx;
    employee_writer w;
    w.name_first = "Did_not";
    w.name_last = "Provide_initialize_rules";
    // Don't change the state of the db at all (no commit).
}
