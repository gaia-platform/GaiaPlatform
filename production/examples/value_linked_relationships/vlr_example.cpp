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
// We implicitly agree to never use the maximum value of uint32_t as a parent ID,
// so to disconnect children from parents, we set their parent IDs to this value.
const uint32_t unused_parent_id = std::numeric_limits<uint32_t>::max();

const uint32_t parent_id = 1;

void insert_parent_and_children()
{
    auto_transaction_t txn{auto_transaction_t::no_auto_begin};

    try
    {
        parent_t::insert_row(parent_id);
        child_t::insert_row(1, parent_id);
        child_t::insert_row(2, parent_id);
        child_t::insert_row(3, parent_id);
        txn.commit();
    }
    catch (gaia::db::index::unique_constraint_violation& uniqueness_violated)
    {
        gaia_log::app().error("{}", uniqueness_violated.what());
    }
}

void delete_all_records_in_db()
{
    auto_transaction_t txn{auto_transaction_t::no_auto_begin};

    for (auto parent = *parent_t::list().begin(); parent; parent = *parent_t::list().begin())
    {
        auto parent_w = parent.writer();
        // Implicitly disconnect the parent from its children by changing its ID
        // to an unused parent ID.
        parent_w.id = unused_parent_id;
        parent_w.update_row();
        parent.delete_row();
    }

    for (auto child = *child_t::list().begin(); child; child = *child_t::list().begin())
    {
        child.delete_row();
    }
    txn.commit();
}

int main()
{
    gaia::system::initialize();
    delete_all_records_in_db();

    insert_parent_and_children();
    gaia::system::shutdown();
}