/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <unistd.h>

#include <iostream>

#include "gaia/system.hpp"

#include "gaia_hello.h"

using namespace std;
using namespace gaia::common;

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

    // relate greetings to names
    gaia::db::begin_transaction();
    gaia_id_t c_id = gaia::hello::greetings_t::insert_row("Goodbye Charles!");
    gaia::db::commit_transaction();
    sleep(1);

    gaia::db::begin_transaction();
    auto first_name = *(gaia::hello::names_t::list().begin());
    cout << first_name.name() << endl;

    // List
    auto g = first_name.greetings();
    if (!g)
    {
        cout << "No greeting connected yet." << endl;
    }
    else
    {
        cout << "Error:  there should be no connection yet!" << endl;
    }
    g.connect(c_id);

    gaia::db::commit_transaction();

    gaia::db::begin_transaction();
    first_name = *(gaia::hello::names_t::list().begin());
    g = first_name.greetings();
    cout << "'" << first_name.name() << "' is connected to '" << g.greeting() << "'" << endl;
    gaia::db::commit_transaction();

    gaia::db::begin_transaction();
    for (auto greeting : gaia::hello::greetings_t::list())
    {
        auto name = greeting.names();
        if (name)
        {
            cout << "'" << greeting.greeting() << "' is connected to '" << name.name() << "'" << endl;
        }
        else
        {
            cout << "'" << greeting.greeting() << "' is not connected to a name" << endl;
        }
    }

    gaia::db::commit_transaction();

    // clean everything up

    gaia::db::begin_transaction();
    while (auto n = *(gaia::hello::names_t::list().begin()))
    {
        if (n.greetings())
        {
            n.greetings().disconnect(n.greetings().gaia_id());
        }
        n.delete_row();
    }

    while (auto g = *(gaia::hello::greetings_t::list().begin()))
    {
        g.delete_row();
    }

    gaia::db::commit_transaction();

    gaia::system::shutdown();

    cout << "Hello example has shut down." << endl;
}
