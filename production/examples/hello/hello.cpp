////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <iostream>

#include "gaia/system.hpp"

#include "gaia_hello.h"

int main()
{
    std::cout << "Hello example is running..." << std::endl;

    gaia::system::initialize();

    gaia::db::begin_transaction();
    gaia::hello::names_t::insert_row("Alice");
    gaia::hello::names_t::insert_row("Bob");
    gaia::hello::names_t::insert_row("Charles");
    gaia::db::commit_transaction();

    gaia::system::shutdown();

    std::cout << "Hello example has shut down." << std::endl;
}
