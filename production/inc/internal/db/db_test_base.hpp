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

#include "gaia/db/gaia_db.hpp"
#include "db_test_helpers.hpp"
#include "db_types.hpp"
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
    bool is_client_managing_session()
    {
        return m_client_manages_session;
    }

private:
    bool m_client_manages_session;

protected:
    static void SetUpTestSuite()
    {
        gaia_log::initialize({});
    }

    explicit db_test_base_t(bool client_manages_session)
        : m_client_manages_session(client_manages_session){};

    db_test_base_t()
        : db_test_base_t(false){};

    // Since ctest always launches each gtest in a new process, there is no point
    // to defining separate SetUpTestSuite/TearDownTestSuite methods.  However, tests
    // that need to do one-time initialization when running outside of ctest
    // can provide SetUpTestSuite/TearDownTestSuite methods and call reset_server()
    // themselves.  These tests should also override SetUp() and TearDown()
    // methods to ensure that the server isn't reset for every test case.

    void SetUp() override
    {
        reset_server();
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
