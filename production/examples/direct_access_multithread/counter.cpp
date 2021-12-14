/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

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
 * which is handled by exponential backoff with full jitter strategy.
 *
 * @param counter_id The id of the record to be updated.
 */
void increase_count_worker(gaia::common::gaia_id_t counter_id)
{
    // Every thread must begin a new session.
    gaia::db::begin_session();

    // Exponential backoff parameters.
    constexpr int c_max_retries = 5;
    int retries = 0;

    std::random_device rd;
    std::mt19937 gen(rd());

    while (true)
    {
        try
        {
            // Tries increasing the counter value by 1.
            auto_transaction_t txn;
            counter_t counter = counter_t::get(counter_id);
            counter_writer counter_w = counter.writer();
            counter_w.count++;
            counter_w.update_row();
            txn.commit();

            // The thread exists as soon as the transaction is
            // successfully committed.
            break;
        }
        catch (const gaia::db::transaction_update_conflict& e)
        {
            // A transaction conflict has occurred.

            if (retries >= c_max_retries)
            {
                // Stop the execution if the transaction cannot be completed
                // within c_max_retries attempts.
                gaia::db::end_session();
                throw e;
            }

            gaia_log::app().info("A transaction update conflict has occurred!");

            // Wait an exponentially increasing time before retrying
            // the update transaction.
            int sleep_millis = (1 << retries) * 10;
            std::uniform_int_distribution<int> dist(0, sleep_millis - 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(dist(gen)));
            retries++;
        }
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

    // Starts 5 worker threads all updating the same counter.
    // You can try increasing this number and see how the
    // behavior change.
    constexpr int num_workers = 5;
    std::vector<std::thread> workers;
    workers.reserve(num_workers);

    for (int i = 0; i < num_workers; i++)
    {
        workers.emplace_back(increase_count_worker, counter_id);
    }

    // Wait for all the workers to complete.
    for (auto& worker : workers)
    {
        worker.join();
    }

    // Check the final value of the counter. It should be the same as
    // the number of workers.
    gaia::db::begin_transaction();
    counter_t counter = counter_t::get(counter_id);
    gaia_log::app().info("Final count is: {}", counter.count());
    gaia::db::commit_transaction();

    gaia::system::shutdown();
}
