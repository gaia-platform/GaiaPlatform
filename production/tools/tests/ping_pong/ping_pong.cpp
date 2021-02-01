/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <thread>
#include <vector>

#include "gaia/db/db.hpp"
#include "gaia/logger.hpp"
#include "gaia/system.hpp"

#include "constants.hpp"
#include "gaia_ping_pong.h"

using namespace std;
using namespace gaia::ping_pong;

constexpr int c_log_heartbeat_frequency = 10000;

/**
 * Worker thread that will set ping_pong.status = "ping" whenever it is == "pong".
 *
 * @param ping_pong_id id of the ping_pong object
 */
void worker(gaia::common::gaia_id_t ping_pong_id);

int main(int argc, char* argv[])
{
    int num_workers;

    if (argc == 1)
    {
        num_workers = 1;
    }
    else if (argc == 2)
    {
        num_workers = stoi(argv[0]);
        assert(num_workers > 0);
    }
    else
    {
        abort();
    }

    // You may want to tune the application behavior by changing the configuration.
    gaia::system::initialize("gaia.conf", "gaia_log.conf");
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
}

void worker(gaia::common::gaia_id_t ping_pong_id)
{
    int count = 0;

    while (true)
    {
        count++;
        try
        {
            gaia::db::begin_transaction();
            auto ping_pong = ping_pong_t::get(ping_pong_id);

            if (count % c_log_heartbeat_frequency == 0)
            {
                // If, for whatever reason, the next condition is never met, at least we log something.
                gaia_log::app().info("Main:{} iteration:{}", ping_pong.status(), count);
            }

            if (strcmp(ping_pong.status(), c_pong) == 0)
            {
                gaia_log::app().info("Main:{}->{} iteration:{} txn_id:{}", ping_pong.status(), c_ping, count, 0 /* txn_id not exposed yet */);
                auto ping_pong_writer = ping_pong.writer();
                ping_pong_writer.status = c_ping;
                ping_pong_writer.update_row();
            }
            gaia::db::commit_transaction();
        }
        catch (const gaia::db::transaction_update_conflict& ex)
        {
            gaia_log::app().error("Main:{} txn:{}", ex.what(), 0 /* txn_id not exposed yet */);
        }
    }
}
