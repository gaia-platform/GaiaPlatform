////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <gaia_clearpath.h>

namespace gaia::clearpath::graph
{

graph_t create_graph(const std::string& uuid);

vertex_t create_vertex(const graph_t& graph, int64_t id, int64_t vertex_type);

edge_t create_edge(int64_t id, const vertex_t& src, const vertex_t& dest);

void clear_data();

} // namespace gaia::clearpath::graph
