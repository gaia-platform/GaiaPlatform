/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gaia/system.hpp"

#include "gaia_hello.h"

using namespace std;

int main()
{
    cout
        << "Hello example is running..."
        << endl;

    gaia::system::initialize();

    gaia::db::begin_transaction();
    gaia::hello::names_t::insert_row("Alice");
    gaia::hello::names_t::insert_row("Bob");
    gaia::hello::names_t::insert_row("Charles");
    gaia::db::commit_transaction();

    gaia::db::begin_transaction();
    auto first_name = *(gaia::hello::names_t::list().begin());
    auto g = first_name.greetings();
    //auto n = g.names();

    gaia::system::shutdown();

    cout << "Hello example has shut down." << endl;
}
