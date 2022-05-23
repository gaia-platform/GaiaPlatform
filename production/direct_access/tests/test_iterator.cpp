////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include "gaia/common.hpp"

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_addr_book.h"

using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::addr_book;

const std::string c_name_1 = "Anastasia";
const std::string c_name_2 = "Bartholomew";

constexpr size_t c_read_wait_in_ms = 0;
constexpr size_t c_update_wait_in_ms = 200;
constexpr size_t c_count_iterations = 10;

bool g_stop_all = false;

class direct_access__iterator__test : public db_catalog_test_base_t
{
protected:
    direct_access__iterator__test()
        : db_catalog_test_base_t("addr_book.ddl", true, true, true)
    {
    }
};

// A reader will keep attempting to read the single record
// in our table in a very tight loop.
//
// The reader will check the size of the table before
// attempting to iterate over its records.
void read_work(size_t id, size_t wait_in_ms)
{
    size_t size = 0;
    std::string name;
    bool found = false;
    size_t iteration_number = 0;

    begin_session();

    // Keep running until a request to stop execution
    // or we reached our iteration count.
    while (!g_stop_all && ++iteration_number < c_count_iterations)
    {
        found = false;

        begin_transaction();

        size = customer_t::list().size();

        // Stop execution if we couldn't find the record.
        if (size == 0)
        {
            g_stop_all = true;
        }

        for (auto customer : customer_t::list())
        {
            name = customer.name();
            found = true;
        }

        // Stop execution if we couldn't find the record.
        if (!found)
        {
            g_stop_all = true;
        }

        commit_transaction();

        // Wait before attempting another read.
        usleep(wait_in_ms);
    }

    begin_transaction();
    for (auto customer : customer_t::list())
    {
        name = customer.name();
        std::cout
            << "R" << id << "> "
            << "Final lookup found: " << name << std::endl;
    }
    commit_transaction();

    end_session();

    std::cout
        << "R" << id << "> " << iteration_number << "/" << size << "/" << name
        << "(" << found << ")" << std::endl;
}

// An updater will periodically update the record's name field
// to one of two different values.
//
// The updater will check the current name value and then will
// try to update it to the other value.
//
// Update conflicts are expected and are just tracked in a counter
// for reporting at the end of the execution.
void update_work(size_t id, size_t wait_in_ms)
{
    size_t size = 0;
    std::string name;
    bool found_name_1 = false;
    bool found_name_2 = false;
    size_t iteration_number = 0;
    size_t update_count = 0;
    size_t conflict_count = 0;

    begin_session();

    // Keep running until a request to stop execution.
    // This is done so that updater threads don't outlast the reader ones.
    while (!g_stop_all && ++iteration_number > 0)
    {
        found_name_1 = false;
        found_name_2 = false;

        try
        {
            begin_transaction();

            size = customer_t::list().size();

            // Stop execution if we couldn't find the record.
            if (size == 0)
            {
                g_stop_all = true;
            }

            for (auto customer : customer_t::list())
            {
                name = customer.name();
                if (name.compare(c_name_1) == 0)
                {
                    found_name_1 = true;
                }
                else if (name.compare(c_name_2) == 0)
                {
                    found_name_2 = true;
                }
            }

            if (found_name_1)
            {
                auto customer_writer = customer_t::list().begin()->writer();
                customer_writer.name = c_name_2;
                customer_writer.update_row();
                ++update_count;
            }
            else if (found_name_2)
            {
                auto customer_writer = customer_t::list().begin()->writer();
                customer_writer.name = c_name_1;
                customer_writer.update_row();
                ++update_count;
            }
            else
            {
                // Stop execution if we couldn't find the record.
                g_stop_all = true;
            }

            commit_transaction();
        }
        catch (const gaia::db::transaction_update_conflict& e)
        {
            ++conflict_count;
        }

        // Wait before attempting another update.
        usleep(wait_in_ms);
    }

    begin_transaction();
    for (auto customer : customer_t::list())
    {
        name = customer.name();
        std::cout
            << "U" << id << "> Final lookup found: " << name << std::endl;
    }
    commit_transaction();

    end_session();

    std::cout
        << "U" << id << "> Iteration: " << iteration_number << "/" << size << "/" << name
        << "[" << update_count << "/" << conflict_count << "]"
        << "(" << found_name_1 << "/" << found_name_2 << ")" << std::endl;
}

TEST_F(direct_access__iterator__test, parallel_read_update)
{
    begin_transaction();
    customer_t::insert_row(c_name_1.c_str(), std::vector<int32_t>());
    commit_transaction();

    begin_transaction();
    for (auto customer : customer_t::list())
    {
        std::cout << "Starting customer name: " << customer.name() << std::endl;
    }
    commit_transaction();

    std::thread reader_thread_1(read_work, 1, c_read_wait_in_ms);
    std::thread reader_thread_2(read_work, 2, c_read_wait_in_ms);
    std::thread reader_thread_3(read_work, 3, c_read_wait_in_ms);
    std::thread reader_thread_4(read_work, 4, c_read_wait_in_ms);
    std::thread updater_thread1(update_work, 1, c_update_wait_in_ms);

    reader_thread_1.join();
    reader_thread_2.join();
    reader_thread_3.join();
    reader_thread_4.join();

    EXPECT_FALSE(g_stop_all);

    g_stop_all = true;

    updater_thread1.join();
}
