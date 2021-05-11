/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <chrono>
#include <cstdlib>

#include <fstream>
#include <memory>
#include <random>
#include <string>
#include <thread>

#include "gtest/gtest.h"

#include "gaia/db/db.hpp"

#include "gaia_internal/common/logger_internal.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/system_error.hpp"
#include "gaia_internal/db/db_client_config.hpp"
#include "gaia_internal/db/db_server_instance.hpp"
#include "gaia_internal/db/db_types.hpp"

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

    static server_instance_t& get_server_instance()
    {
        return s_server_instance;
    }

protected:
    explicit db_test_base_t(bool client_manages_session)
        : m_client_manages_session(client_manages_session), m_disable_persistence(true)
    {
    }

    db_test_base_t()
        : db_test_base_t(false){};

    static void SetUpTestSuite()
    {
        gaia_log::initialize({});

        s_server_instance = server_instance_t{};

        // Make the instance name the default, so that calls to begin_session()
        // will automatically connect to that instance.
        session_opts_t session_opts;
        session_opts.db_instance_name = s_server_instance.instance_name();
        config::set_default_session_opts(session_opts);

        s_server_instance.start();
        s_server_instance.wait_for_init();
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
            s_server_instance.reset_server();
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

protected:
    static inline server_instance_t s_server_instance{};

private:
    bool m_client_manages_session;
    bool m_disable_persistence;
};

} // namespace db
} // namespace gaia
