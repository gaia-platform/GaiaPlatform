/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <chrono>

#include <thread>

#include <gtest/gtest.h>

#include "gaia/common.hpp"

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_addr_book.h"

using namespace std::chrono_literals;

using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::addr_book;

const std::string c_name_1 = "Anastasia";
const std::string c_name_2 = "Bartholomew";
const size_t c_count_iterations = 10;

bool g_stop_all = false;

class test_iterator : public db_catalog_test_base_t
{
protected:
    test_iterator()
        : db_catalog_test_base_t("addr_book.ddl"){};
};

void read_work(size_t id)
{
    size_t size = 0;
    std::string name;
    bool found = false;
    size_t iteration_number = 0;

    gaia::db::begin_session();

    while (!g_stop_all && ++iteration_number < c_count_iterations)
    {
        found = false;

        begin_transaction();

        size = customer_t::list().size();

        if (size == 0)
        {
            g_stop_all = true;
        }

        for (auto customer : customer_t::list())
        {
            name = customer.name();
            found = true;
        }

        if (!found)
        {
            g_stop_all = true;
        }

        commit_transaction();
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

    gaia::db::end_session();

    std::cout
        << "R" << id << "> " << iteration_number << "/" << size << "/" << name
        << "(" << found << ")" << std::endl;
}

void update_work(size_t id)
{
    size_t size = 0;
    std::string name;
    bool found_name_1 = false;
    bool found_name_2 = false;
    size_t iteration_number = 0;
    size_t update_count = 0;
    size_t conflict_count = 0;

    gaia::db::begin_session();

    while (!g_stop_all && ++iteration_number > 0)
    {
        found_name_1 = false;
        found_name_2 = false;

        try
        {
            begin_transaction();

            size = customer_t::list().size();

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
                g_stop_all = true;
            }

            commit_transaction();
        }
        catch (const gaia::db::transaction_update_conflict& e)
        {
            ++conflict_count;
        }

        // Wait before attempting another update.
        std::this_thread::sleep_for(100ms);
        ;
    }

    begin_transaction();
    for (auto customer : customer_t::list())
    {
        name = customer.name();
        std::cout
            << "U" << id << "> Final lookup found: " << name << std::endl;
    }
    commit_transaction();

    gaia::db::end_session();

    std::cout
        << "U" << id << "> Iteration: " << iteration_number << "/" << size << "/" << name
        << "[" << update_count << "/" << conflict_count << "]"
        << "(" << found_name_1 << "/" << found_name_2 << ")" << std::endl;
}

TEST_F(test_iterator, parallel_read_update)
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

    std::thread reader_thread_1(read_work, 1);
    std::thread reader_thread_2(read_work, 2);
    std::thread reader_thread_3(read_work, 3);
    std::thread reader_thread_4(read_work, 4);
    std::thread updater_thread1(update_work, 1);

    reader_thread_1.join();
    reader_thread_2.join();
    reader_thread_3.join();
    reader_thread_4.join();

    EXPECT_FALSE(g_stop_all);

    g_stop_all = true;

    updater_thread1.join();
}
