/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <filesystem>
#include <iostream>
#include <thread>

#include <gtest/gtest.h>
#include <libexplain/execve.h>
#include <libexplain/fork.h>
#include <libexplain/kill.h>
#include <sys/prctl.h>

#include <gaia_internal/common/config.hpp>
#include <gaia_internal/common/logger_internal.hpp>
#include <gaia_internal/common/system_error.hpp>
#include <gaia_internal/db/db_client_config.hpp>
#include <gaia_internal/db/db_server_instance.hpp>

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::config;

namespace fs = std::filesystem;

// Ensures both the server and the client use the environment variables.
class db_server_env_test : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        gaia_log::initialize({});
    }

    /// Starts the gaia_db_server passing instance_name and data_dir as
    /// environment variables.
    void start_server(std::string instance_name, std::string data_dir)
    {
        m_server_pid = ::fork();

        if (m_server_pid < 0)
        {
            const char* reason = ::explain_fork();
            throw_system_error(reason);
        }
        else if (m_server_pid == 0)
        {
            // Kills the child process (gaia_db_sever) after the parent dies (current process).
            // This must be put right after ::fork() and before ::execve().
            // This works well with ctest where each test is run as a separated process.
            if (-1 == ::prctl(PR_SET_PDEATHSIG, SIGKILL))
            {
                throw_system_error("prctl() failed!");
            }

            std::string gaia_db_instance_env = gaia_fmt::format("{}={}", c_instance_name_string_env, instance_name);
            std::string gaia_db_data_dir_env = gaia_fmt::format("{}={}", c_data_dir_string_env, data_dir);

            char* argv[] = {nullptr};

            char* envp[] = {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
                const_cast<char*>(gaia_db_instance_env.c_str()),
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
                const_cast<char*>(gaia_db_data_dir_env.c_str()),
                nullptr};

            std::string gaia_db_server_path = server_instance_config_t::find_server_path();

            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            if (-1 == ::execve(gaia_db_server_path.c_str(), argv, envp))
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
                const char* reason = ::explain_execve(gaia_db_server_path.c_str(), argv, envp);
                throw_system_error(reason);
            }
        }
    }

    void kill_server()
    {
        if (-1 == ::kill(m_server_pid, SIGKILL))
        {
            const char* reason = ::explain_kill(m_server_pid, SIGKILL);
            gaia::common::throw_system_error(reason);
        }
    }

private:
    pid_t m_server_pid;
};

TEST_F(db_server_env_test, instance_name_from_env)
{

    std::string instance_name = server_instance_config_t::generate_instance_name();
    std::string data_dir = server_instance_config_t::generate_data_dir(instance_name);

    start_server(instance_name, data_dir);

    // Wait up to 500ms.
    constexpr int c_max_retry = 100;
    static constexpr int c_poll_interval_millis = 5;

    bool connected = false;

    for (int i = 0; i < c_max_retry; ++i)
    {
        try
        {
            std::string gaia_db_instance_env = gaia_fmt::format("{}={}", c_instance_name_string_env, instance_name);

            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            ::putenv(const_cast<char*>(gaia_db_instance_env.c_str()));

            // The client should pick up the instance name from the ENV
            gaia::db::begin_session();
            connected = true;
        }
        catch (gaia::common::system_error& e)
        {
            if (e.get_errno() == ECONNREFUSED)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(c_poll_interval_millis));
                continue;
            }
            else
            {
                throw;
            }
        }
        break;
    }

    ASSERT_TRUE(connected) << "It was not possible to connect the the gaia_db_server instance named " << instance_name;
    ASSERT_TRUE(fs::exists(data_dir)) << "The gaia_db_server did not created the expected data dir " << data_dir;

    // Kill the server and cleanup the directory.
    kill_server();

    for (const auto& entry : fs::directory_iterator(data_dir))
    {
        fs::remove_all(entry.path());
    }
}
