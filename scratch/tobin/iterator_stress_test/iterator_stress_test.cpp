/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <vector>
#include <thread>

#include "gaia/system.hpp"

#include "gaia_iterator_stress_test.h"

using gaia::iterator_stress_test::my_table_t;

int main()
{
    std::cerr << "iterator_stress_test is running..." << std::endl;

    gaia::system::initialize();

    gaia::db::begin_transaction();
    gaia::iterator_stress_test::my_table_t::insert_row("my_value");
    gaia::db::commit_transaction();

    constexpr size_t c_num_threads = 16;
    std::vector<std::thread> threads;

    for (size_t i = 0; i < c_num_threads; ++i)
    {
        threads.emplace_back([i]() {
            gaia::db::begin_session();
            while (true)
            {
                gaia::db::begin_transaction();
                my_table_t my_table = *(my_table_t::list().begin());
                auto my_value = my_table.my_value();
                // std::cerr << "Thread " << i << ": Value of my_table.my_value: " << my_value << std::endl;
                gaia::db::commit_transaction();
            }
            gaia::db::end_session();
        });
    }

    for (auto& t : threads)
    {
        t.join();
    }
    gaia::system::shutdown();

    std::cerr << "iterator_stress_test has shut down." << std::endl;
}
