/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <csignal>

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

#include "gaia/common.hpp"
#include "gaia/db/db.hpp"
#include "gaia/logger.hpp"
#include "gaia/system.hpp"

#include "constants.hpp"
#include "gaia_ping_pong.h"

using namespace std;
using namespace gaia::ping_pong;

constexpr int c_log_heartbeat_frequency = 10000;
static atomic_bool g_enable_workers = true;

/**
 * Worker thread that will set ping_pong.status = "ping" whenever it is == "pong".
 *
 * @param ping_pong_id id of the ping_pong object
 */
void worker(gaia::common::gaia_id_t ping_pong_id);

/**
 * Stop workers execution.
 */
void stop_workers_handler(int signal);

int main(int argc, char* argv[])
{
    int num_workers = 1;

    if (argc == 2)
    {
        num_workers = stoi(argv[0]);
        assert(num_workers > 0);
    }
    else if (argc > 2)
    {
        cout << "Usage: ping_pong [num_workers]" << endl;
        abort();
    }

    // Termination handlers.
    struct sigaction sigbreak;
    sigbreak.sa_handler = &stop_workers_handler;
    sigemptyset(&sigbreak.sa_mask);
    sigbreak.sa_flags = 0;
    sigaction(SIGINT, &sigbreak, nullptr);
    sigaction(SIGTERM, &sigbreak, nullptr);

    // Users may want to tune the Gaia behavior by changing the default configuration.
    // Eg. change the number of the rule engine threads.
    gaia::system::initialize();
    gaia_log::app().info("Starting Ping Pong example with {} workers.", num_workers);

    gaia::db::begin_transaction();
    auto ping_pong_id = ping_pong_t::insert_row(c_pong);
    gaia::db::commit_transaction();

    vector<thread> workers;

    for (int i = 0; i < num_workers; i++)
    {
        workers.emplace_back(worker, ping_pong_id);
    }

    for (thread& worker : workers)
    {
        worker.join();
    }

    gaia::system::shutdown();
    cout << "Ping pong application terminated" << endl;
}

void worker(gaia::common::gaia_id_t ping_pong_id)
{
    gaia::db::begin_session();
    int iteration_count = 0;

    // We don't have access to txn_id at this point, but we may add it later.
    uint64_t txn_id = 0;

    while (g_enable_workers)
    {
        iteration_count++;
        try
        {
            gaia::db::begin_transaction();
            auto ping_pong = ping_pong_t::get(ping_pong_id);

            if (iteration_count % c_log_heartbeat_frequency == 0)
            {
                // If, for whatever reason, the next condition is never met, at least we log something.
                gaia_log::app().info("Main:'{}' iteration:'{}'", ping_pong.status(), iteration_count);
            }

            if (strcmp(ping_pong.status(), c_pong) == 0)
            {
                gaia_log::app().info("Main:'{}'->'{}' iteration:'{}' txn_id:'{}'", ping_pong.status(), c_ping, iteration_count, txn_id);
                auto ping_pong_writer = ping_pong.writer();
                ping_pong_writer.status = c_ping;
                ping_pong_writer.update_row();
            }
            gaia::db::commit_transaction();
        }
        catch (const gaia::db::transaction_update_conflict& ex)
        {
            gaia_log::app().error("Main:'{}' txn:'{}'", ex.what(), txn_id);
        }
    }
    gaia::db::end_session();
}

void stop_workers_handler(int signal)
{
    std::cout << "Caught signal '" << signal << "'." << endl;

    g_enable_workers = false;
}
