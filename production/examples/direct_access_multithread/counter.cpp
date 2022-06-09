////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <chrono>
#include <random>
#include <thread>
#include <vector>

#include <gaia/db/db.hpp>
#include <gaia/exceptions.hpp>
#include <gaia/logger.hpp>
#include <gaia/system.hpp>

#include "gaia_counter.h"

using namespace gaia::counter;

using gaia::common::gaia_id_t;
using gaia::direct_access::auto_transaction_t;

///
/// Direct Access API multithreading example.
///
/// Demonstrates usage of the Gaia Direct Access API in a
/// multithreaded application and how to deal with transaction
/// update conflicts.
///

/**
 * Run concurrently by multiple threads, increases the counter
 * value by 1. This is likely to cause a transaction_update_conflict
 * which is caught and logged.
 *
 * @param counter_id The id of the record to be updated.
 */
void increase_count_worker(gaia::common::gaia_id_t counter_id)
{
    // Every thread must begin a new session.
    gaia::db::begin_session();

    try
    {
        // Attempts to increase the counter value by 1.
        auto_transaction_t txn;
        counter_t counter = counter_t::get(counter_id);
        counter_writer counter_w = counter.writer();
        counter_w.count++;
        counter_w.update_row();
        txn.commit();
    }
    catch (const gaia::db::transaction_update_conflict& e)
    {
        // In this example, for simplicity, we print a log message.
        // In a production environment, you should recover from this
        // failure in accordance with your application requirements.
        gaia_log::app().info("A transaction update conflict has occurred!");
    }

    gaia::db::end_session();
}

int main()
{
    gaia::system::initialize();

    // Creates a counter with value 0.
    gaia::db::begin_transaction();
    gaia_id_t counter_id = counter_t::insert_row(0);
    gaia::db::commit_transaction();

    // Starts 5 worker threads that all update the same counter.
    // You can try increasing this number and see how the
    // behavior changes.
    constexpr size_t num_workers = 10;
    std::vector<std::thread> workers;

    for (size_t i = 0; i < num_workers; i++)
    {
        workers.emplace_back(increase_count_worker, counter_id);
    }

    // Wait for all the workers to complete.
    for (auto& worker : workers)
    {
        worker.join();
    }

    // Print the final value of the counter. It should be less than
    // num_workers because of the transaction update conflicts.
    gaia::db::begin_transaction();
    counter_t counter = counter_t::get(counter_id);
    gaia_log::app().info("{} workers succeeded out of {}", counter.count(), num_workers);
    gaia::db::commit_transaction();

    gaia::system::shutdown();
}
