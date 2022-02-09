////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <gaia/db/db.hpp>
#include <gaia/direct_access/auto_transaction.hpp>
#include <gaia/logger.hpp>
#include <gaia/system.hpp>

#include "gaia_clearpath.h"
#include "graph.hpp"
#include "vertex_types.hpp"

using namespace gaia::direct_access;
using namespace gaia::clearpath;
using namespace gaia::clearpath::graph;

int main()
{
    gaia::system::initialize();

    // The no_auto_restart argument prevents beginning a new transaction
    // when the current one is committed.
    auto_transaction_t txn{auto_transaction_t::no_auto_restart};

    clear_data();

    graph_t graph = create_graph("aaaaaaaa-0000-0000-0000-000000000000");

    vertex_t v1 = create_vertex(graph, 1, vertex_type::c_lidar_scan);
    vertex_t v2 = create_vertex(graph, 2, vertex_type::c_lidar_scan);
    vertex_t v3 = create_vertex(graph, 3, vertex_type::c_magnetic_line);

    edge_t e1 = create_edge(1, v1, v2);
    edge_t e2 = create_edge(2, v2, v3);

    // Get all graph vertexes and edges.

    gaia_log::app().info("=== Printing all vertexes from Graph({})  ===", graph.uuid());

    for (const vertex_t& v : graph.vertices())
    {
        gaia_log::app().info(" Vertex({})", v.id());
    }

    gaia_log::app().info("=== Printing all edges from Graph({}) ===", graph.uuid());

    for (const edge_t& e : graph.edges())
    {
        gaia_log::app().info(" Edge({}): ({}) --> ({})", e.id(), e.src().id(), e.dest().id());
    }

    gaia_log::app().info("=== Get all vertexes with type lidar_scan   ===", graph.uuid());

    for (const vertex_t& v : vertex_t::list()
                                 .where(vertex_expr::type == vertex_type::c_lidar_scan))
    {
        gaia_log::app().info(" Vertex({})", v.id());
    }

    gaia_log::app().info("=== Get the vertex with id 2 and type lidar_scan ===", graph.uuid());

    auto vertex_it = vertex_t::list()
                         .where(
                             vertex_expr::id == 2
                             && vertex_expr::type == vertex_type::c_lidar_scan);

    if (vertex_it.begin() == vertex_it.end())
    {
        throw std::runtime_error("Impossible to find vertex with id 1 and type lidar_scan");
    }

    gaia_log::app().info(" Vertex({})", vertex_it.begin()->id());

    gaia_log::app().info("=== Get all edges with src_id or dest_id equals 1 ===", graph.uuid());

    auto edge_it = edge_t::list()
                       .where(
                           edge_expr::dest == v1 || edge_expr::src == v1);

    for (const edge_t& e : edge_it)
    {
        gaia_log::app().info(" Edge({}): ({}) --> ({})", e.id(), e.src().id(), e.dest().id());
    }

    txn.commit();

    gaia::system::shutdown();
}
