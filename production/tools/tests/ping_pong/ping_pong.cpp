/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia/db/db.hpp"
#include "gaia/logger.hpp"
#include "gaia/system.hpp"

#include "constants.hpp"
#include "gaia_ping_pong.h"

using namespace std;
using namespace gaia::ping_pong;

int main()
{
    gaia::system::initialize("gaia.conf", "gaia_log.conf");
    gaia_log::app().info("Ping Pong example is running...");

    gaia::db::begin_transaction();
    auto ping_pong_id = ping_pong_t::insert_row(c_ping);
    auto ping_pong = ping_pong_t::get(ping_pong_id);
    gaia::db::commit_transaction();

    int count = 0;

    while (true)
    {
        count++;
        try
        {
            gaia::db::begin_transaction();
            ping_pong = ping_pong_t::get(ping_pong_id);

            if (count % 1000 == 0)
            {
                gaia_log::app().info("Main:{} iteration:{}", ping_pong.status(), count);
            }

            if (strcmp(ping_pong.status(), c_pong) == 0)
            {
                auto ping_pong_writer = ping_pong.writer();
                ping_pong_writer.status = c_ping;
                ping_pong_writer.update_row();
            }
            gaia::db::commit_transaction();
        }
        catch (const gaia::db::transaction_update_conflict& ex)
        {
            gaia_log::app().error("{} iteration:{}", ex.what(), count);
        }
    }

    gaia::system::shutdown();
}
