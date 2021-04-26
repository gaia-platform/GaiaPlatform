/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cstdlib>
#include <ctime>

#include <shared_mutex>
#include <string>
#include <thread>

#include "gtest/gtest.h"

#include "gaia/db/db.hpp"
#include "gaia/logger.hpp"

#include "gaia_internal//common/logger_internal.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/system_error.hpp"
#include "gaia_internal/common/timer.hpp"

#include "gaia_addr_book.h"
#include "schema_loader.hpp"

using namespace gaia::db;
using namespace gaia::addr_book;

using gaia_timer_t = gaia::common::timer_t;

static constexpr uint32_t c_num_server_instances = 10;
static constexpr uint32_t c_num_employees = 10;
static constexpr uint32_t c_sleep_micros = 100000;
static constexpr uint32_t c_max_reader_wait_seconds = 10;

class multiple_server_instances_test : public ::testing::Test
{
public:
    std::string get_server_exec_path()
    {
        return "/home/simone/repos/GaiaPlatform/production/cmake-build-debug/db/core/gaia_db_server";
    }

protected:
    static void SetUpTestSuite()
    {
        gaia_log::initialize({});
    }
};

class server_instance_t
{
public:
    server_instance_t(const std::string& server_exec_path, const std::string& instance_name)
        : m_server_exec_path(server_exec_path), m_instance_name(instance_name)
    {
    }

    void start_server()
    {
        gaia_log::app().info("Starting server instance {}", m_instance_name, m_server_pid);
        const char* const command[] = {m_server_exec_path.c_str(), "--instance-name", m_instance_name.c_str(), "--disable-persistence", NULL};

        if (0 == (m_server_pid = ::fork()))
        {
            if (-1 == ::execve(command[0], (char**)command, NULL))
            {
                FAIL() << "Failed to execute " << m_server_exec_path.c_str();
            }
        }

        gaia_log::app().info("Server instance {} started with pid:{}", m_instance_name, m_server_pid);
    }

    void kill_server()
    {
        gaia_log::app().info("Killing server instance {} and pid:{}", m_instance_name, m_server_pid);

        ::system((std::string("kill -9 ") + std::to_string(m_server_pid)).c_str());
    }

    void wait_for_server()
    {
        constexpr int c_poll_interval_millis = 10;
        constexpr int c_print_error_interval = 1000;
        // Initialize to 1 to avoid printing a spurious wait message.
        int counter = 1;

        // Wait for server to initialize.
        while (true)
        {
            try
            {
                gaia_log::app().info("Waiting for server instance {} attempt:{}", m_instance_name, counter);

                session_opts_t session_opts;
                session_opts.instance_name = m_instance_name;

                begin_session(session_opts);
            }
            catch (gaia::common::system_error& ex)
            {
                if (ex.get_errno() == ECONNREFUSED)
                {
                    if (counter % c_print_error_interval == 0)
                    {
                        gaia_log::sys().warn(
                            "Cannot connect to Gaia Server; the 'gaia_db_server' process may not be running!");
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
        end_session();
    }

    std::string instance_name()
    {
        return m_instance_name;
    }

private:
    std::string m_server_exec_path;
    std::string m_instance_name;
    ::pid_t m_server_pid;
};

class client_writer_t
{
public:
    client_writer_t(const std::string& instance_name, std::shared_mutex& gaia_parser_lock)
        : m_instance_name(instance_name), m_gaia_parser_lock(gaia_parser_lock)
    {
    }

    void do_write()
    {
        gaia_log::app().info("Starting writer for instance {}", m_instance_name);

        session_opts_t session_opts;
        session_opts.instance_name = m_instance_name;

        begin_session(session_opts);

        // TODO Apparently the gaia parser is not thread safe.
        //  I don't know if this is by design or a bug.
        //  @chuan to review (if it is a bug I will file a JIRA)
        std::unique_lock lock(m_gaia_parser_lock);
        schema_loader_t::instance().load_schema("addr_book.ddl");
        lock.unlock();

        begin_transaction();
        for (uint32_t i = 0; i < c_num_employees; i++)
        {
            employee_writer ew;
            ew.name_first = std::string("name_") + std::to_string(i);
            ew.name_last = std::string("surname_") + std::to_string(i);
            ew.insert_row();
        }

        commit_transaction();
        end_session();

        gaia_log::app().info("Exiting writer for instance {}", m_instance_name);
    }

private:
    std::string m_instance_name;
    std::shared_mutex& m_gaia_parser_lock;
};

class client_reader_t
{
public:
    client_reader_t(const std::string& instance_name)
        : m_instance_name(instance_name)
    {
    }

    void do_read()
    {
        gaia_log::app().info("Starting reader for instance {}", m_instance_name);

        session_opts_t session_opts;
        session_opts.instance_name = m_instance_name;

        begin_session(session_opts);

        uint32_t num_employees = 0;

        auto start_time = gaia_timer_t::get_time_point();

        // Wait up to 10 seconds for the writer thread to write.
        while (num_employees == 0)
        {
            begin_transaction();
            num_employees = employee_t::list().size();

            if (num_employees == 0)
            {
                ::usleep(c_sleep_micros);
            }

            auto elapsed_time = gaia_timer_t::get_duration(start_time);

            if (gaia_timer_t::ns_to_s(elapsed_time) >= c_max_reader_wait_seconds)
            {
                break;
            }
            commit_transaction();
        }

        end_session();

        ASSERT_EQ(c_num_employees, num_employees);
        gaia_log::app().info("Exiting reader for instance {}", m_instance_name);
    }

private:
    std::string m_instance_name;
};

// https://stackoverflow.com/a/440240
std::string gen_random_str(const int len)
{

    std::string tmp_s;
    static const char alphanum[] = "0123456789"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz";

    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i)
        tmp_s += alphanum[::rand() % (sizeof(alphanum) - 1)];

    return tmp_s;
}

TEST_F(multiple_server_instances_test, multiple_instacnes)
{
    std::vector<server_instance_t> server_instances;

    for (uint32_t i = 0; i < c_num_server_instances; i++)
    {
        std::string instance_name = gen_random_str(5).append("_").append(std::to_string(i));
        server_instance_t gaia_db_instance{get_server_exec_path(), instance_name};
        gaia_db_instance.start_server();
        gaia_db_instance.wait_for_server();
        server_instances.push_back(gaia_db_instance);
    }

    std::vector<std::thread> writer_threads;

    std::shared_mutex gaia_parser_lock;

    for (auto& server_instance : server_instances)
    {
        client_writer_t writer{server_instance.instance_name(), gaia_parser_lock};
        writer_threads.emplace_back(&client_writer_t::do_write, writer);
    }

    std::vector<std::thread> reader_threads;

    for (auto& server_instance : server_instances)
    {
        client_reader_t reader{server_instance.instance_name()};
        reader_threads.emplace_back(&client_reader_t::do_read, reader);
    }

    for (auto& t : writer_threads)
    {
        t.join();
    }

    for (auto& t : reader_threads)
    {
        t.join();
    }

    for (auto& server_instance : server_instances)
    {
        server_instance.kill_server();
    }
}
