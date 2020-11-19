/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <chrono>
#include <cstdlib>

#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>

#include "gtest/gtest.h"

#include "db_test_helpers.hpp"
#include "db_types.hpp"
#include "gaia_db.hpp"
#include "retail_assert.hpp"
#include "system_error.hpp"

using namespace gaia::common;
using namespace gaia::db;

namespace gaia
{
namespace db
{

class db_test_base_t : public ::testing::Test
{
public:
private:
    bool m_client_manages_session;
    bool m_disable_persistence;

protected:
    db_test_base_t(bool client_manages_session, bool disable_persistence = true)
        : m_client_manages_session(client_manages_session), m_disable_persistence(disable_persistence)
    {
    }

    db_test_base_t()
        : db_test_base_t(false)
    {
    }

    // Since ctest always launches each gtest in a new process, there is no point
    // to defining separate SetUpTestSuite/TearDownTestSuite methods.  However, tests
    // that need to do one-time initialization when running outside of ctest
    // can provide SetUpTestSuite/TearDownTestSuite methods and call reset_server()
    // themselves.  These tests should also override SetUp() and TearDown()
    // methods to ensure that the server isn't reset for every test case.

    void SetUp() override
    {
        gaia_log::initialize({});
        // The server will only reset on SIGHUP if persistence is disabled.
        if (m_disable_persistence)
        {
            gaia_log::db().info("Resetting server before running test.");
            reset_server();
        }
        else
        {
            gaia_log::db().warn("Not resetting server before test because persistence is enabled.");
        }
        if (!m_client_manages_session)
        {
            begin_session();
        }
    }

    void TearDown() override
    {
        if (!m_client_manages_session)
        {
            end_session();
        }
    }
};

} // namespace db
} // namespace gaia
