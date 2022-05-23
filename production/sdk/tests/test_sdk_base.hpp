////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

// Do not include any internal headers to ensure that
// the code included in this file doesn't have a dependency
// on non-public APIs.

#include <gtest/gtest.h>

#include "gaia/common.hpp"
#include "gaia/db/db.hpp"
#include "gaia/db/events.hpp"
#include "gaia/logger.hpp"
#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"

#include "gaia_addr_book.h"

bool g_initialize_rules_called = false;

constexpr size_t c_wait_server_millis = 100;

// Smoke tests public APIs, to ensure they work as expected and the symbols
// are exported (https://github.com/gaia-platform/GaiaPlatform/pull/397);
class sdk_base : public ::testing::Test
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
        system("pkill -f -KILL gaia_db_server ");
    }

    void SetUp() override
    {
        // Load the schema in the database in a "public way".
        system("../catalog/gaiac/gaiac addr_book.ddl");
        gaia::system::initialize("./gaia.conf", "./gaia_log.conf");
    }

    void TearDown() override
    {
        gaia::system::shutdown();
    }
};

template <typename T_ex, typename... T_args>
void test_exception(T_args... args)
{
    bool thrown = false;
    try
    {
        throw T_ex(args...);
    }
    catch (const T_ex& ex)
    {
        thrown = true;
    }

    ASSERT_TRUE(thrown) << "An exception should have been thrown";
}
