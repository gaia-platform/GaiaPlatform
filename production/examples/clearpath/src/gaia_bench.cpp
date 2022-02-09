////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <chrono>

#include <gaia/db/db.hpp>
#include <gaia/direct_access/auto_transaction.hpp>
#include <gaia/logger.hpp>
#include <gaia/system.hpp>

#include "gaia_clearpath.h"

using namespace gaia::direct_access;
using namespace gaia::clearpath;
using namespace std::chrono;

const constexpr uint32_t c_insert_buffer_stmts = 1000;
const constexpr uint32_t c_num_vertexes = 10000000;

void generate_data()
{
    auto_transaction_t txn;
    for (int i = 0; i < c_num_vertexes; i++)
    {
        gaia_log::app().info("Iteration: {}...", i);

        vertex_writer vertex_w;
        vertex_w.id = i;
        vertex_w.type = (i % 5);
        vertex_w.insert_row();

        if (i > 1)
        {
            edge_writer edge_w;
            edge_w.id = i;
            edge_w.src_id = (i - 1);
            edge_w.dest_id = i;
            edge_w.insert_row();
        }

        if (i % c_insert_buffer_stmts == 0 || i == c_num_vertexes - 1)
        {
            gaia_log::app().info("Inserting data: {}...", i);
            txn.commit();
        }
    }
}

int main()
{
    gaia::system::initialize();

    steady_clock::time_point begin = std::chrono::steady_clock::now();
    generate_data();
    steady_clock::time_point end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<milliseconds>(end - begin).count();

    gaia_log::app().info("Time to insert {} statements: {}ms", c_num_vertexes, elapsed);

    gaia::system::shutdown();
}
