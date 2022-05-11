/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

// Don't provide a definition of `initialize_rules` to ensure we don't
// get a link error.  The other test (test_sdk.cpp) tests that a strong
// reference will override the weak reference and take precedence.

#include <thread>

#include <gtest/gtest.h>

#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"

#include "gaia_internal/catalog/catalog.hpp"

#include "gaia_addr_book.h"

using namespace std;

using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::addr_book;
using namespace gaia::db::triggers;

constexpr size_t c_wait_server_millis = 100;

class sdk__no_init_rules : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        // Starts the server in a "public way".
        system("../db/core/gaia_db_server --persistence disabled &");
        // Wait for the server.
        std::this_thread::sleep_for(std::chrono::milliseconds(c_wait_server_millis));
    }

    static void TearDownTestSuite()
    {
        system("pkill -f -KILL gaia_db_server");
    }
};

TEST_F(sdk__no_init_rules, app_check)
{
    gaia::system::initialize();
    {
        auto_transaction_t tx;
        employee_writer w;
        w.name_first = "Did_not";
        w.name_last = "Provide_initialize_rules";
        // Don't write to the db because catalog is not properly populated.
        // Don't change the state of the db at all (no commit).
    }
    gaia::system::shutdown();
}
