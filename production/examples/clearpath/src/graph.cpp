////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "graph.hpp"

namespace gaia::clearpath::graph
{

graph_t create_graph(const std::string& uuid)
{
    return graph_t::get(graph_t::insert_row(uuid.c_str()));
}

vertex_t create_vertex(const graph_t& graph, int64_t id, int64_t vertex_type)
{
    vertex_writer vertex_w;
    vertex_w.id = id;
    vertex_w.type = vertex_type;
    vertex_w.graph_uuid = graph.uuid();

    return vertex_t::get(vertex_w.insert_row());
}

edge_t create_edge(int64_t id, const vertex_t& src, const vertex_t& dest)
{
    if (src.graph_uuid() == dest.graph_uuid())
    {
        throw std::runtime_error("You cannot create an edge from vertexes in different graphs!");
    }

    edge_writer edge_w;
    edge_w.id = id;
    edge_w.src_id = src.id();
    edge_w.dest_id = dest.id();
    edge_w.graph_uuid = src.graph_uuid();

    return edge_t::get(edge_w.insert_row());
}

template <typename T_type>
void clear_table()
{
    for (auto obj_it = T_type::list().begin();
         obj_it != T_type::list().end();)
    {
        auto next_obj_it = obj_it++;
        next_obj_it->delete_row();
    }
}

void clear_data()
{
    clear_table<edge_t>();
    clear_table<point_edge_t>();
    clear_table<vertex_t>();
    clear_table<graph_t>();
}

} // namespace gaia::clearpath::graph
