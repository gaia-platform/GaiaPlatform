/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "db_server_cmd.hpp"

#include <unistd.h>

#include <filesystem>
#include <iostream>
#include <thread>

#include <sys/prctl.h>
#include <sys/wait.h>

#include "gaia/db/db.hpp"
#include "gaia/exceptions.hpp"
#include "gaia/logger.hpp"

#include "resource_manager.hpp"

namespace fs = std::filesystem;

static constexpr char c_db_core_folder_name[] = "db/core";
static constexpr int c_poll_interval_millis = 5;
static constexpr int c_print_error_interval = 1000;

// The name of the default gaia instance.
constexpr char c_default_instance_name[] = "gaia_default_instance";

// The name of the SE server binary.
constexpr char c_db_server_exec_name[] = "gaia_db_server";

namespace gaia
{
namespace cmd
{

server_instance_config_t gaia::cmd::server_instance_config_t::get_default_config()
{
    return server_instance_config_t{
        .server_exec_path = find_server_path(),
        .disable_persistence = true,
        .instance_name = c_default_instance_name,
        .skip_catalog_integrity_check = false,
        .data_dir = ""};
}

fs::path server_instance_config_t::find_server_path()
{
    std::string server_path = gaia_fmt::format("{}/{}", c_db_core_folder_name, c_db_server_exec_name);
    return gaia::test::find_resource(server_path);
}

std::string server_instance_config_t::generate_data_dir(const std::string& instance_name)
{
    fs::path tmp_dir = fs::temp_directory_path() / "gaia" / instance_name;
    fs::create_directories(tmp_dir);
    return tmp_dir.string();
}

void server_instance_t::start(bool wait_for_init)
{
    // ASSERT_PRECONDITION(!m_is_initialized, "The server must not be initialized");

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

    gaia_spdlog::debug("Starting server instance:{} with command: '{}'.", instance_name(), command_str);

    m_server_pid = ::fork();

    if (m_server_pid < 0)
    {
        throw std::runtime_error("fork() failed!");
    }
    else if (m_server_pid == 0)
    {
        // Kills the child process (gaia_db_sever) after the parent dies (current process).
        // This must be put right after ::fork() and before ::execve().
        // This works well with ctest where each test is run as a separated process.
        if (-1 == ::prctl(PR_SET_PDEATHSIG, SIGKILL))
        {
            throw std::runtime_error("prctl() failed!");
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        if (-1 == ::execve(command[0], const_cast<char**>(command.data()), nullptr))
        {
            throw std::runtime_error("execve() failed while executing gaia_db_server!");
        }
    }

    gaia_spdlog::debug("Server instance:{} started with pid:{}.", instance_name(), m_server_pid);
    m_is_initialized = true;

    if (wait_for_init)
    {
        server_instance_t::wait_for_init();
    }
}

void server_instance_t::stop()
{
    // ASSERT_PRECONDITION(m_is_initialized, "The server must be initialized");

    gaia_spdlog::debug("Killing server instance:{} and pid:{}.", instance_name(), m_server_pid);

    if (::kill(m_server_pid, SIGKILL) == -1)
    {
        throw std::runtime_error("kill() failed while sending SIGKILL to gaia_db_server!");
    }

    // Wait for the termination, this should be almost immediate
    // hence no need to have a separate public function.
    int status;
    pid_t return_pid = ::waitpid(m_server_pid, &status, 0);

    if (return_pid == -1)
    {
        throw std::runtime_error("waitpid() failed while waiting for gaia_db_server!");
    }
    else if (return_pid == 0)
    {
        // waitpid() returns zero only if WNOHANG option is specified.
        // We don't use WNOHANG but leaving this branch to prevent regressions.
        throw std::runtime_error("The db server process should be killed!");
    }

    m_is_initialized = false;
    m_server_pid = -1;
}

void server_instance_t::restart(bool wait_for_init)
{
    stop();
    start(wait_for_init);
}

void server_instance_t::wait_for_init()
{
    // ASSERT_PRECONDITION(m_is_initialized, "The server must be initialized");

    // Initialize to 1 to avoid printing a spurious wait message.
    int counter = 1;

    // Wait for server to initialize.
    while (true)
    {
        try
        {
            gaia_spdlog::trace("Waiting for Gaia instance:{}...", instance_name());

            gaia::db::begin_session();
        }
        catch (gaia::db::server_connection_failed& ex)
        {
            if (counter % c_print_error_interval == 0)
            {
                gaia_spdlog::warn(
                    "Cannot connect to Gaia instance:{}; the '{}' process may not be running!",
                    instance_name(),
                    c_db_server_exec_name);
                counter = 1;
            }
            else
            {
                counter++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(c_poll_interval_millis));
            continue;
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

void server_instance_t::delete_data_dir()
{
    // ASSERT_PRECONDITION(!m_conf.data_dir.empty(), "data_dir should be set.");
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

    if (m_conf.server_exec_path.empty())
    {
        throw common::gaia_exception("Impossible to find gaia_db_server executable!");
    }

    strings.push_back(m_conf.server_exec_path.c_str());
    strings.push_back("--instance-name");
    strings.push_back(m_conf.instance_name.c_str());

    if (m_conf.disable_persistence)
    {
        // ASSERT_PRECONDITION(m_conf.data_dir.empty(), "data_dir must be empty when persistence is disabled.");
        strings.push_back("--persistence");
        strings.push_back("disabled");
    }
    else
    {
        // ASSERT_PRECONDITION(!m_conf.data_dir.empty(), "data_dir must be specified when persistence is enabled.");
        strings.push_back("--data-dir");
        strings.push_back(m_conf.data_dir.c_str());
    }

    if (m_conf.skip_catalog_integrity_check)
    {
        strings.push_back("--skip-catalog-integrity-checks");
    }

    strings.push_back(nullptr);
    return strings;
}
} // namespace cmd
} // namespace gaia
