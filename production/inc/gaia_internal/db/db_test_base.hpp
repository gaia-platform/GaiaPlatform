////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <cstdlib>

#include <chrono>
#include <fstream>
#include <memory>
#include <random>
#include <string>
#include <thread>

#include "gtest/gtest.h"

#include "gaia/db/db.hpp"

#include "gaia_internal/common/logger.hpp"
#include "gaia_internal/db/db.hpp"
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
    bool is_managing_session()
    {
        return m_is_managing_session;
    }

    bool start_ddl_session()
    {
        return m_start_ddl_session;
    }

    static server_instance_t& get_server_instance()
    {
        return s_server_instance;
    }

protected:
    explicit db_test_base_t(
        bool is_managing_session = true,
        bool start_ddl_session = false)
        : m_is_managing_session(is_managing_session)
        , m_start_ddl_session(start_ddl_session)
        , m_disable_persistence(true)
    {
    }

    static void SetUpTestSuite()
    {
        SetUpTestSuite({});
    }

    // Test suites can customize logger behavior by passing in a logger configuration file.
    // This must be done in a test specific SetupTestSuite function which then must call
    // db_test_base_t::SetupTestSuite with the logger file path.  If the test does not provide
    // its own implementation of SetupTestSuite then default logging with an "empty" configuration
    // above.
    static void SetUpTestSuite(const std::string& log_conf_path)
    {
        gaia_log::initialize(log_conf_path);

        server_instance_config_t conf = server_instance_config_t::get_new_instance_config();
        conf.skip_catalog_integrity_check = true;
        s_server_instance = server_instance_t(conf);

        // Make the instance name the default, so that calls to begin_session()
        // will automatically connect to that instance.
        config::session_options_t session_options;
        session_options.db_instance_name = s_server_instance.instance_name();
        session_options.skip_catalog_integrity_check = s_server_instance.skip_catalog_integrity_check();
        config::set_default_session_options(session_options);

        s_server_instance.start();
    }

    static void TearDownTestSuite()
    {
        gaia_log::shutdown();
    }

    // Since ctest always launches each gtest in a new process, there is no point
    // to defining separate SetUpTestSuite/TearDownTestSuite methods.  However, tests
    // that need to do one-time initialization when running outside ctest
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

        if (m_is_managing_session)
        {
            if (m_start_ddl_session)
            {
                begin_ddl_session();
            }
            else
            {
                begin_session();
            }
        }
    }

    void TearDown() override
    {
        if (m_is_managing_session)
        {
            end_session();
        }
    }

protected:
    static inline server_instance_t s_server_instance{};

private:
    bool m_is_managing_session;
    bool m_start_ddl_session;
    bool m_disable_persistence;
};

} // namespace db
} // namespace gaia
