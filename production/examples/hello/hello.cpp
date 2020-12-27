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
        << endl
        << "Please wait for greetings to be printed, then terminate execution by pressing Enter."
        << endl;

    gaia::system::initialize();

    gaia::db::begin_transaction();
    gaia::hello::names_t::insert_row("Alice");
    gaia::hello::names_t::insert_row("Bob");
    gaia::hello::names_t::insert_row("Charles");
    gaia::db::commit_transaction();

    // Wait for user input.
    cin.get();

    gaia::system::shutdown();

    cout << "Hello example has shut down." << endl;
}
