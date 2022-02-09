////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <gaia/logger.hpp>
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

    for (auto person = person_t::list().begin(); person != person_t::list().end();)
    {
        (*person++).delete_row();
    }
    for (auto floor = floor_t::list().begin(); floor != floor_t::list().end();)
    {
        (*floor++).delete_row();
    }

    txn.commit();
}

/**
 * An example of using Value Linked References when updating fields in
 * a record, which automatically reconnects it to related records.
 * It starts by inserting all the necessary records.
 */
void vlr_example_usage()
{
    auto_transaction_t txn{};

    // Floors inserted with their numbers and department names.
    floor_t::insert_row(0, "Lobby");
    floor_t::insert_row(1, "Sales");
    floor_t::insert_row(2, "Engineering");
    floor_t::insert_row(3, "Admin");

    // Insert people at certain floors. Bill starts at floor 0: the lobby.
    person_t person = person_t::get(person_t::insert_row("Bill", 0));
    txn.commit();

    // We need a writer to change a person's field values.
    person_writer person_w = person.writer();

    // Move the person up a floor three times.
    for (int i = 0; i < 3; ++i)
    {
        // Changing their floor is as easy as incrementing floor_number.
        // With VLRs, this automatically reconnects the person to the next floor.
        person_w.floor_number++;
        person_w.update_row();
        txn.commit();

        // Now that the person is implicitly connected to a floor through a VLR,
        // we can directly access that floor using the current_floor reference.
        gaia_log::app().info("{} has arrived at: {}", person.name(), person.current_floor().department());
    }
}

int main()
{
    gaia::system::initialize();
    delete_all_records_from_tables();

    vlr_example_usage();

    gaia::system::shutdown();
}
