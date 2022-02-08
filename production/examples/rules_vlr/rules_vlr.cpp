/////////////////////////////////////////////
// Copyright (c) 2021 Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
/////////////////////////////////////////////

#include <gaia/system.hpp>

#include "gaia_rules_vlr.h"

using namespace gaia::rules_vlr;
using gaia::direct_access::auto_transaction_t;

/**
 * Deletes all records from all tables.
 * We want to start this app with a consistent blank slate every time.
 */
void delete_all_records_from_tables()
{
    auto_transaction_t txn{auto_transaction_t::no_auto_restart};

    for (auto student = student_t::list().begin(); student != student_t::list().end();)
    {
        (*student++).delete_row();
    }
    for (auto dorm_room = dorm_room_t::list().begin(); dorm_room != dorm_room_t::list().end();)
    {
        (*dorm_room++).delete_row();
    }
    txn.commit();
}

/**
 * Insert dorm rooms into the database.
 */
void create_dorm_rooms()
{
    auto_transaction_t txn{};

    dorm_room_t::insert_row(100, "A120", 3);
    dorm_room_t::insert_row(101, "B304", 2);
    dorm_room_t::insert_row(102, "C217", 2);

    txn.commit();
}

/**
 * Insert students into the database. This will trigger a rule that assigns
 * students to dorms that are below capacity.
 */
void insert_new_students()
{
    auto_transaction_t txn{};

    const uint32_t c_unused_dorm_room_id = 0;

    student_t::insert_row(1000, "Todd", c_unused_dorm_room_id);
    student_t::insert_row(1001, "Jane", c_unused_dorm_room_id);
    student_t::insert_row(1002, "John", c_unused_dorm_room_id);
    student_t::insert_row(1003, "Sarah", c_unused_dorm_room_id);
    student_t::insert_row(1004, "Ned", c_unused_dorm_room_id);
    student_t::insert_row(1005, "Dave", c_unused_dorm_room_id);

    txn.commit();
}

int main()
{
    gaia::system::initialize();
    delete_all_records_from_tables();

    create_dorm_rooms();
    insert_new_students();

    gaia::system::shutdown();
}
