/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

// Don't provide a definition of `initialize_rules` to ensure we don't
// get a link error.  The other test (test_sdk.cpp) tests that a strong
// reference will override the weak reference and take precedence.

#include "gtest/gtest.h"

#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"

#include "gaia_internal/catalog/catalog.hpp"

#include "gaia_addr_book.h"

using namespace std;

using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::addr_book;
using namespace gaia::db::triggers;

TEST(sdk_test_no_init_rules, app_check)
{
    gaia::system::initialize();
    {
        auto_transaction_t tx;
        employee_writer w;
        w.name_first = "Did_not";
        w.name_last = "Provide_initialize_rules";
        w.insert_row();
        // Don't change the state of the db at all (no commit).
    }
    gaia::system::shutdown();
}
