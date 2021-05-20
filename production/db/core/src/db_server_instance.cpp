/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/db/db_server_instance.hpp"

#include <csignal>

#include <filesystem>
#include <iostream>

#include <spdlog/fmt/fmt.h>
#include <sys/prctl.h>
#include <sys/wait.h>

#include "gaia/db/db.hpp"
#include "gaia/exceptions.hpp"

#include "gaia_internal/common/logger_internal.hpp"
#include "gaia_internal/common/random.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/system_error.hpp"
#include "gaia_internal/db/db_client_config.hpp"
#include "gaia_internal/db/gaia_db_internal.hpp"

namespace fs = std::filesystem;

static constexpr char c_db_core_folder_name[] = "db/core";
static constexpr int c_poll_interval_millis = 5;
static constexpr int c_print_error_interval = 1000;

namespace gaia
{
namespace db
{

server_instance_config_t gaia::db::server_instance_config_t::get_default()
{
    return server_instance_config_t{
        .server_exec_path = find_server_path(),
        .instance_name = generate_instance_name(),
        .disable_persistence = true,
        .data_dir = ""};
}

fs::path server_instance_config_t::find_server_path()
{
    fs::path current_path = fs::current_path();
    fs::path db_path = fs::path(current_path) / c_db_core_folder_name;

    while (!fs::exists(db_path) && current_path.has_root_path() && current_path != current_path.root_path())
    {
        current_path = current_path.parent_path();
        db_path = fs::path(current_path) / c_db_core_folder_name;
    }

    fs::path db_exec_path = db_path / c_db_server_exec_name;

    if (!fs::exists(db_exec_path))
    {
        throw common::gaia_exception("Database server path does not exist.");
    }

    return db_exec_path;
}

std::string server_instance_config_t::generate_instance_name()
{
    constexpr int c_random_suffix_size = 4;

    std::string executable_name;
    std::ifstream("/proc/self/comm") >> executable_name;
    std::string current_exe_path = fmt::format("{}/{}", std::filesystem::current_path().string(), executable_name);

    std::size_t path_hash = std::hash<std::string>{}(current_exe_path);
    std::string random_str = common::gen_random_str(c_random_suffix_size);

    return fmt::format("{}_{}_{}", executable_name, path_hash, random_str);
}

std::string server_instance_config_t::generate_data_dir(const std::string& instance_name)
{
    fs::path tmp_dir = fs::temp_directory_path() / "gaia" / instance_name;
    fs::create_directories(tmp_dir);
    return tmp_dir.string();
}

void server_instance_t::start(bool wait_for_init)
{
    ASSERT_PRECONDITION(!m_is_initialized, "The server must not be initialized");

    std::vector<const char*> command = get_server_command_and_argument();

    // TODO this should be wrapped into a if_debug_enabled()
    std::string command_str;
    for (const char* part : command)
    {
        if (part)
        {
            command_str.append(part);
            command_str.append(" ");
        }
    }

    gaia_log::sys().debug("Starting server instance:{} with command: '{}'.", instance_name(), command_str);

    if (0 == (m_server_pid = ::fork()))
    {
        // Kills the child process (gaia_db_sever) after the parent dies (current process).
        // This must be put right after ::fork() and before ::execve().
        // This works well with ctest where each test is run as a separated process.
        ::prctl(PR_SET_PDEATHSIG, SIGKILL);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        if (-1 == ::execve(command[0], const_cast<char**>(command.data()), nullptr))
        {
            common::throw_system_error(fmt::format("execve failed: {}!", m_conf.server_exec_path.string()));
        }
    }

    gaia_log::sys().debug("Server instance:{} started with pid:{}.", instance_name(), m_server_pid);
    m_is_initialized = true;

    if (wait_for_init)
    {
        server_instance_t::wait_for_init();
    }
}

void server_instance_t::stop(bool wait_for_stop)
{
    ASSERT_PRECONDITION(m_is_initialized, "The server must be initialized");

    gaia_log::sys().debug("Killing server instance:{} and pid:{}.", instance_name(), m_server_pid);

    ::system(fmt::format("kill -KILL {}", m_server_pid).c_str());

    if (wait_for_stop)
    {
        wait_for_termination();
    }

    m_is_initialized = false;
    m_server_pid = -1;
}

void server_instance_t::restart(bool wait_for_init_and_stop)
{
    stop(wait_for_init_and_stop);
    start(wait_for_init_and_stop);
}

void server_instance_t::reset_server(bool wait_for_init)
{
    ASSERT_PRECONDITION(m_is_initialized, "The server must be initialized");

    gaia_log::sys().debug("Resetting server instance:{} and pid:{}.", instance_name(), m_server_pid);

    // We need to allow enough time after the signal is sent for the process to
    // receive and process the signal.
    constexpr int c_wait_signal_millis = 10;
    // We need to drop all client references to shared memory before resetting the server.
    // NB: this cannot be called within an active session!
    gaia::db::clear_shared_memory();
    // Reinitialize the server (forcibly disconnects all clients and clears database).
    // Resetting the server will cause Recovery to be skipped. Recovery will only occur post
    // server process reboot.
    ::system(fmt::format("kill -HUP {}", m_server_pid).c_str());
    // Wait a bit for the server's listening socket to be closed.
    // (Otherwise, a new session might be accepted after the signal has been sent
    // but before the server has been reinitialized.)
    std::this_thread::sleep_for(std::chrono::milliseconds(c_wait_signal_millis));

    if (wait_for_init)
    {
        server_instance_t::wait_for_init();
    }
}

void server_instance_t::wait_for_init()
{
    ASSERT_PRECONDITION(m_is_initialized, "The server must be initialized");

    // Initialize to 1 to avoid printing a spurious wait message.
    int counter = 1;

    // Wait for server to initialize.
    while (true)
    {
        try
        {
            gaia_log::sys().trace("Waiting for Gaia instance:{}...", instance_name());

            gaia::db::config::session_options_t session_options;
            session_options.db_instance_name = instance_name();

            gaia::db::config::set_default_session_options(session_options);

            gaia::db::begin_session();
        }
        catch (gaia::common::system_error& e)
        {
            if (e.get_errno() == ECONNREFUSED)
            {
                if (counter % c_print_error_interval == 0)
                {
                    gaia_log::sys().warn(
                        "Cannot connect to Gaia instance:{}; the '{}' process may not be running!", instance_name(), c_db_server_exec_name);
                    counter = 1;
                }
                else
                {
                    counter++;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(c_poll_interval_millis));
                continue;
            }
            else
            {
                throw;
            }
        }
        catch (...)
        {
            throw;
        }
        break;
    }
    // This was just a test connection, so disconnect.
    gaia::db::end_session();
}

void server_instance_t::wait_for_termination()
{
    // Initialize to 1 to avoid printing a spurious wait message.
    int counter = 1;

    pid_t return_pid = -1;

    while (return_pid != m_server_pid)
    {
        int status;
        return_pid = ::waitpid(m_server_pid, &status, WNOHANG);

        if (return_pid == -1)
        {
            gaia::common::throw_system_error("waitpid failed!");
        }
        else if (return_pid == 0)
        {
            // Process still running.
            std::this_thread::sleep_for(std::chrono::milliseconds(c_poll_interval_millis));

            if (counter % c_print_error_interval == 0)
            {
                gaia_log::sys().warn(
                    "The server instance:{} is still running, waiting for termination...", instance_name());
                counter = 1;
            }
            else
            {
                counter++;
            }
        }
    }
}

void server_instance_t::delete_data_dir()
{
    ASSERT_PRECONDITION(!m_conf.data_dir.empty(), "data_dir should be set.");
    for (const auto& entry : fs::directory_iterator(m_conf.data_dir))
    {
        fs::remove_all(entry.path());
    }
}

std::string server_instance_t::instance_name()
{
    return m_conf.instance_name;
}

bool server_instance_t::is_initialized()
{
    return m_is_initialized;
}

std::vector<const char*> server_instance_t::get_server_command_and_argument()
{
    std::vector<const char*> strings;

    strings.push_back(m_conf.server_exec_path.c_str());
    strings.push_back("--instance-name");
    strings.push_back(m_conf.instance_name.c_str());

    if (m_conf.disable_persistence)
    {
        ASSERT_PRECONDITION(m_conf.data_dir.empty(), "data_dir must be empty when persistence is disabled.");
        strings.push_back("--disable-persistence");
    }
    else
    {
        ASSERT_PRECONDITION(!m_conf.data_dir.empty(), "data_dir must be specified when persistence is enabled.");
        strings.push_back("--data-dir");
        strings.push_back(m_conf.data_dir.c_str());
    }

    strings.push_back(nullptr);
    return strings;
}
} // namespace db
} // namespace gaia
