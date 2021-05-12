/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <shared_mutex>
#include <string>
#include <thread>

#include "gtest/gtest.h"

#include "gaia/db/db.hpp"
#include "gaia/logger.hpp"

#include "gaia_internal/common/logger_internal.hpp"
#include "gaia_internal/common/timer.hpp"
#include "gaia_internal/db/db_server_instance.hpp"

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
    multiple_server_instances_test()
    {
    }

protected:
    static void SetUpTestSuite()
    {
        gaia_log::initialize({});
    }

private:
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
        gaia_log::app().debug("Starting writer for instance {}", m_instance_name);

        session_options_t session_options;
        session_options.db_instance_name = m_instance_name;

        begin_session(session_options);

        // TODO The gaia parser does not support running
        //  multiple DDL from different threads.
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

        gaia_log::app().debug("Exiting writer for instance {}", m_instance_name);
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
        gaia_log::app().debug("Starting reader for instance {}", m_instance_name);

        session_options_t session_options;
        session_options.db_instance_name = m_instance_name;

        begin_session(session_options);

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
        gaia_log::app().debug("Exiting reader for instance {}", m_instance_name);
    }

private:
    std::string m_instance_name;
};

TEST_F(multiple_server_instances_test, multiple_instacnes)
{
    std::vector<server_instance_t> server_instances;

    for (uint32_t i = 0; i < c_num_server_instances; i++)
    {
        server_instance_t gaia_db_instance;
        gaia_db_instance.start();
        gaia_db_instance.wait_for_init();
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
        server_instance.stop();
    }
}
