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
using namespace gaia::hello;

void dump_db()
{
    for (auto n : names_t::list())
    {
        printf("%s\n", n.name());

        if (n.preferred_greeting())
        {
            printf("Preferred greeting: '%s', for '%s'\n", n.preferred_greeting().greeting(), n.preferred_greeting().name_1().name());
        }
        else
        {
            printf("No preferred greeting set for '%s'.\n", n.name());
        }
        printf("Standard greetings:'\n");
        for (auto g : n.greetings())
        {
            printf("\t'%s', for '%s'\n", g.greeting(), g.name_M().name());
        }
    }
}

void clear_db()
{
    while (auto n = *(names_t::list().begin()))
    {
        n.greetings().clear();
        n.preferred_greeting().disconnect();
        n.delete_row();
    }

    while (auto g = *(greetings_t::list().begin()))
    {
        g.delete_row();
    }
}

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

    sleep(1);

    gaia::db::begin_transaction();
    dump_db();
    gaia::db::commit_transaction();

    gaia::db::begin_transaction();
    auto n = *(names_t::list().begin());

    // let's switch a preferred greeting with a standard one.
    for (auto g : n.greetings())
    {
        n.preferred_greeting().connect(g);
        n.greetings().remove(g);
        break;
    }
    //n.preferred_greeting().disconnect();

    /* this doesn't work to disconnect.  not sure why yet
    auto g = *(greetings_t::list().begin());
    printf("Greeting '%s\n", g.greeting());
    if (g.name_1())
    {
        printf("Name_1: '%s'\n", g.name_1().name());
        // dies here with Gaia type '12' has no relationship for the offset '0'
        // Maybe I'm not setting up the 1:1 relationship correctly
        g.name_1().disconnect();
    }
    */

    //n.preferred_greeting().disconnect();
    dump_db();
    gaia::db::commit_transaction();

    gaia::db::begin_transaction();
    clear_db();
    gaia::db::commit_transaction();

    /*

    // relate greetings to names
    gaia::db::begin_transaction();
    gaia_id_t c_id = gaia::hello::greetings_t::insert_row("Goodbye Charles!");
    gaia::db::commit_transaction();
    sleep(1);

    gaia::db::begin_transaction();
    auto first_name = *(gaia::hello::names_t::list().begin());
    cout << first_name.name() << endl;

    // List
    auto g = first_name.preferred_greeting();
    if (!g)
    {
        cout << "No preferred greeting connected yet." << endl;
    }
    else
    {
        cout << "Error:  there should be no connection yet!" << endl;
    }
    g.connect(c_id);

    gaia::db::commit_transaction();

    gaia::db::begin_transaction();
    first_name = *(gaia::hello::names_t::list().begin());
    g = first_name.preferred_greeting();
    cout << "'" << first_name.name() << "' is connected to '" << g.greeting() << "'" << endl;
    gaia::db::commit_transaction();

    gaia::db::begin_transaction();
    for (auto greeting : gaia::hello::greetings_t::list())
    {
        auto name = greeting.name_1();
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
        if (n.preferred_greeting())
        {
            //n.greetings().disconnect(n.greetings().gaia_id());
            n.preferred_greeting().disconnect();
        }
        n.delete_row();
    }

    while (auto g = *(gaia::hello::greetings_t::list().begin()))
    {
        g.delete_row();
    }

    gaia::db::commit_transaction();

    // Now let's do the M side

*/
    gaia::system::shutdown();

    cout << "Hello example has shut down." << endl;
}
