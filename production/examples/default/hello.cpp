/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gaia/system.hpp"

#include "gaia.h"

using namespace std;

int main()
{
    cout
        << "Hello example is running..."
        << endl;

    gaia::system::initialize();

    gaia::db::begin_transaction();
    gaia::names_t::insert("Alice");
    gaia::names_t::insert("Bob");
    gaia::names_t::insert("Charles");
    gaia::db::commit_transaction();

    gaia::system::shutdown();

    cout << "Hello example has shut down." << endl;
}
