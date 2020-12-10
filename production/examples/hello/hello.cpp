/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <unistd.h>

#include <iostream>

#include "gaia/system.hpp"
#include "gaia_hello.h"

using namespace std;

int main(int argc, const char** argv)
{
    cout
        << "Hello example is running..."
        << endl
        << "Please wait for greeting to be printed, then terminate execution by pressing any key."
        << endl;

    gaia::system::initialize("gaia.conf");

    gaia::db::begin_transaction();
    gaia::hello::names_t::insert_row("George");
    gaia::db::commit_transaction();

    // Wait for user input.
    cin.get();

    gaia::system::shutdown();

    cout << "Hello example has shut down." << endl;
}
