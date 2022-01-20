/////////////////////////////////////////////
// Copyright (c) 2021 Gaia Platform LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
/////////////////////////////////////////////

#include <limits>

#include <gaia/db/db.hpp>
#include <gaia/exceptions.hpp>
#include <gaia/logger.hpp>
#include <gaia/system.hpp>

#include "gaia_vlr_example.h"

using namespace gaia::vlr_example;
using gaia::direct_access::auto_transaction_t;

void reset_database_to_clean_slate()
{
    auto_transaction_t txn{auto_transaction_t::no_auto_begin};

    for (auto person = *person_t::list().begin(); person; person = *person_t::list().begin())
    {
        person.delete_row();
    }
    for (auto floor = *floor_t::list().begin(); floor; floor = *floor_t::list().begin())
    {
        floor.delete_row();
    }
    txn.commit();
}

void vlr_example_usage()
{
    auto_transaction_t txn{};
    // Floors inserted with their numbers and department names.
    floor_t::insert_row(0, "Lobby");
    floor_t::insert_row(1, "Sales");
    floor_t::insert_row(2, "Engineering");
    floor_t::insert_row(3, "Admin");
    // Insert people at certain floors. Bill starts at floor 0: the lobby.
    person_t bill = person_t::get(person_t::insert_row("Bill", 0));
    person_t::insert_row("Todd", 1);
    person_t::insert_row("Jane", 1);
    person_t::insert_row("John", 2);
    person_t::insert_row("Sarah", 2);
    person_t::insert_row("Ned", 2);
    person_t::insert_row("Dave", 3);
    txn.commit();

    // We need a writer to change Bill's field values.
    person_writer bill_w = bill.writer();

    // Move Bill up a floor three times.
    for (int i = 0; i < 3; ++i)
    {
        // Changing his floor is as easy as incrementing floor_num.
        // With VLRs, this automatically reconnects Bill to the correct floor.
        bill_w.floor_num++;
        bill_w.update_row();
        txn.commit();
    }

    // Move him back down to the lobby.
    bill_w.floor_num = 0;
    bill_w.update_row();
    txn.commit();
}

int main()
{
    gaia::system::initialize();
    reset_database_to_clean_slate();

    vlr_example_usage();
    gaia::system::shutdown();
}
