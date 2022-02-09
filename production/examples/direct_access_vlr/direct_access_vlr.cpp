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

    for (auto elevator = elevator_t::list().begin(); elevator != elevator_t::list().end();)
    {
        (*elevator++).delete_row();
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

    // Floors inserted with their numbers and descriptions.
    floor_t::insert_row(0, "Lobby");
    floor_t::insert_row(1, "Sales");
    floor_t::insert_row(2, "Engineering");
    floor_t::insert_row(3, "Admin");

    // Insert an elevator with an ID of 1 that starts at floor 0, the lobby.
    elevator_t elevator = elevator_t::get(elevator_t::insert_row(1, 0));
    txn.commit();

    // We need a writer to change a elevator's field values.
    elevator_writer elevator_w = elevator.writer();

    // Move the elevator up a floor three times.
    for (int i = 0; i < 3; ++i)
    {
        // Changing its floor is as easy as incrementing floor_number.
        // With VLRs, this automatically reconnects the elevator to the next floor.
        elevator_w.floor_number++;
        elevator_w.update_row();
        txn.commit();

        // Now that the elevator is implicitly connected to a floor through a VLR,
        // we can directly access that floor using the current_floor reference.
        floor_t current_floor = elevator.current_floor();
        gaia_log::app().info("Elevator {} has arrived at floor {}: {}", elevator.elevator_id(), current_floor.floor_number(), current_floor.description());
    }
}

int main()
{
    gaia::system::initialize();
    delete_all_records_from_tables();

    vlr_example_usage();

    gaia::system::shutdown();
}
