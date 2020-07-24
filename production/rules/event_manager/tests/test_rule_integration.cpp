/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include <unordered_map>
#include "gtest/gtest.h"
#include "rules.hpp"
#include "gaia_system.hpp"
#include "addr_book_gaia_generated.h"
#include "db_test_helpers.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::direct_access;
using namespace gaia::rules;
using namespace std;

/**
 * Table Rule functions.
 *
 * Make sure the rule operations are commutative since we don't guarantee
 * the order that rules are fired.  Table rules write to the passed in "row"
 * data.
 */
void rule1(const rule_context_t* /*context*/)
{
}

 extern "C"
 void initialize_rules()
 {
 }

/**
 * Google test fixture object.  This class is used by each
 * test case below.  SetUp() is called before each test is run
 * and TearDown() is called after each test case is done.
 */
class rule_integration_test : public ::testing::Test
{
protected:
    static void SetUpTestSuite() {
        start_server();
    }

    static void TearDownTestSuite() {
        stop_server();
    }
};

TEST_F(rule_integration_test, test_1)
{
}
