/////////////////////////////////////////////
// Copyright (c) 2021 Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
/////////////////////////////////////////////

#include <gaia/system.hpp>

#include "gaia_direct_access_vlr.h"

using namespace gaia::direct_access_vlr;
using gaia::direct_access::auto_transaction_t;

/**
 * Deletes all records from all tables.
 * We want to start this app with a consistent blank slate every time.
 */
void delete_all_records_from_tables()
{
    auto_transaction_t txn{auto_transaction_t::no_auto_restart};

    for (auto person = *person_t::list().begin(); person; person = *person_t::list().begin())
    {
        person.delete_row();
    }
    for (auto level = *level_t::list().begin(); level; level = *level_t::list().begin())
    {
        level.delete_row();
    }
    txn.commit();
}

/**
 * An example of using Value-Linked Relationships when updating fields in
 * a record, which automatically reconnects it to related records.
 * It starts by inserting all the necessary records.
 */
void vlr_example_usage()
{
    auto_transaction_t txn{};

    // Levels inserted with their numbers and department names.
    level_t::insert_row(0, "Lobby");
    level_t::insert_row(1, "Sales");
    level_t::insert_row(2, "Engineering");
    level_t::insert_row(3, "Admin");

    // Insert people at certain levels. Bill starts at level 0: the lobby.
    person_t person = person_t::get(person_t::insert_row("Bill", 0));
    person_t::insert_row("Todd", 1);
    person_t::insert_row("Jane", 1);
    person_t::insert_row("John", 2);
    person_t::insert_row("Sarah", 2);
    person_t::insert_row("Ned", 2);
    person_t::insert_row("Dave", 3);
    txn.commit();

    // We need a writer to change a person's field values.
    person_writer person_w = person.writer();

    // Move the person up a level three times.
    for (int i = 0; i < 3; ++i)
    {
        // Changing their level is as easy as incrementing level_number.
        // With VLRs, this automatically reconnects the person to the next level.
        person_w.level_number++;
        person_w.update_row();
        txn.commit();
    }

    // Move them back down to the lobby.
    person_w.level_number = 0;
    person_w.update_row();
    txn.commit();
}

int main()
{
    gaia::system::initialize();
    delete_all_records_from_tables();

    vlr_example_usage();

    gaia::system::shutdown();
}
