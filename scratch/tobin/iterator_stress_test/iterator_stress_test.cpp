/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

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

    while (true)
    {
        gaia::db::begin_transaction();
        my_table_t my_table = *(my_table_t::list().begin());
        auto my_value = my_table.my_value();
        std::cerr << "Value of my_table.my_value: " << my_value << std::endl;
        gaia::db::commit_transaction();
    }

    gaia::system::shutdown();

    std::cerr << "iterator_stress_test has shut down." << std::endl;
}
