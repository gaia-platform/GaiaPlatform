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

// We cannot delete records (yet) from a Value-Linked Relationship
// without first disconnecting the records by changing the value of a linked field.
// We implicitly agree to never use int's maximum value as a floor number,
// so to disconnect people from floors, we set their floor numbers to this value.
const int unused_floor_num = std::numeric_limits<int>::max();

person_t bill;

void insert_floors_and_people()
{
    auto_transaction_t txn{auto_transaction_t::no_auto_begin};

    try
    {
        floor_t::insert_row(0, "Lobby");
        floor_t::insert_row(1, "Sales");
        floor_t::insert_row(2, "Engineering");
        floor_t::insert_row(3, "Admin");

        bill = person_t::get(person_t::insert_row("Bill", 0));
        person_t::insert_row("Todd", 1);
        person_t::insert_row("Jane", 1);
        person_t::insert_row("John", 2);
        person_t::insert_row("Sarah", 2);
        person_t::insert_row("Ned", 2);
        person_t::insert_row("Dave", 3);

        txn.commit();
    }
    catch (gaia::db::index::unique_constraint_violation& uniqueness_violated)
    {
        gaia_log::app().error("{}", uniqueness_violated.what());
    }
}

void delete_all_floors_and_people()
{
    auto_transaction_t txn{auto_transaction_t::no_auto_begin};

    for (auto floor = *floor_t::list().begin(); floor; floor = *floor_t::list().begin())
    {
        auto floor_w = floor.writer();
        // Implicitly disconnect the floor from the people by changing its number
        // to an unused floor number.
        floor_w.num = unused_floor_num;
        floor_w.update_row();
        floor.delete_row();
    }

    for (auto person = *person_t::list().begin(); person; person = *person_t::list().begin())
    {
        person.delete_row();
    }
    txn.commit();
}

void move_around_floors()
{
    auto_transaction_t txn{};
    person_writer bill_w = bill.writer();

    for (int i = 0; i < 3; ++i)
    {
        bill_w.floor_num++;
        bill_w.update_row();
        txn.commit();
    }

    bill_w.floor_num = 0;
    bill_w.update_row();
    txn.commit();
}

int main()
{
    gaia::system::initialize();
    delete_all_floors_and_people();

    insert_floors_and_people();
    move_around_floors();
    gaia::system::shutdown();
}