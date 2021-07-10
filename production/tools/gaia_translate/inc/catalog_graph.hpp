/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <map>
#include <string>

#include "lemon/list_graph.h"
#include "lemon/path.h"

#include "gaia/common.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"

namespace gaia
{
namespace translation
{

struct link_t
{
    common::gaia_id_t source_table;
    common::gaia_id_t target_table;
    std::string link_name;
    catalog::relationship_cardinality_t cardinality;
};

struct table_path_t
{
    std::vector<link_t> path;
};

class catalog_graph_t
{
public:
    using graph_t = lemon::ListDigraph;
    using node_t = lemon::ListDigraph::Node;
    using arc_t = lemon::ListDigraph::Arc;
    using path_t = lemon::ListPath<lemon::ListDigraph>;

    catalog_graph_t();

    node_t get_node(common::gaia_id_t table_id);
    std::vector<table_path_t> find_paths(common::gaia_id_t from_table, common::gaia_id_t to_table);
    std::vector<table_path_t> find_shortest_paths(common::gaia_id_t from_table, common::gaia_id_t to_table);

private:
    graph_t m_graph;
    graph_t::NodeMap<common::gaia_id_t> m_node_values;
    graph_t::ArcMap<link_t> m_arc_values;

    std::map<common::gaia_id_t, node_t> m_reverse_node_mapping;
};

} // namespace translation
} // namespace gaia
