/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <stdio.h>

#include <iostream>
#include <string>

#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"
#include "gaia_hello.h"

using namespace std;

int main(int argc, const char** argv)
{
    cout
        << "Hello example is running..."
        << endl
        << "Give it a couple of seconds to print its greeting and then press any key to terminate."
        << endl;

    gaia::system::initialize("gaia.conf");
    gaia::db::begin_transaction();

    gaia::hello::names_t::insert_row("George");

    // gaia::hello::greetings_t::insert_row("Goodbye George!");

    gaia::db::commit_transaction();
    gaia::system::shutdown();

    cout << "Hello example has shut down." << endl;
}
