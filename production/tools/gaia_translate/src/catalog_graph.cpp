/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "catalog_graph.hpp"

#include "lemon/dfs.h"

namespace gaia
{
namespace translation
{

catalog_graph_t::catalog_graph_t()
    : m_graph(), m_node_values(m_graph), m_arc_values(m_graph)
{
    for (catalog::gaia_table_t& table : catalog::gaia_table_t::list())
    {
        node_t node = m_graph.addNode();
        m_node_values[node] = table.gaia_id();
        m_reverse_node_mapping[table.gaia_id()] = node;
    }

    for (catalog::gaia_relationship_t& relationship : catalog::gaia_relationship_t::list())
    {
        node_t parent_node = m_reverse_node_mapping[relationship.parent().gaia_id()];
        node_t child_node = m_reverse_node_mapping[relationship.child().gaia_id()];

        arc_t parent_to_child_arc = m_graph.addArc(parent_node, child_node);
        link_t parent_to_child_link;
        parent_to_child_link.link_name = relationship.to_child_link_name();
        parent_to_child_link.source_table = relationship.parent().gaia_id();
        parent_to_child_link.target_table = relationship.child().gaia_id();
        parent_to_child_link.cardinality = static_cast<catalog::relationship_cardinality_t>(relationship.cardinality());
        m_arc_values[parent_to_child_arc] = parent_to_child_link;

        arc_t child_to_parent_arc = m_graph.addArc(child_node, parent_node);
        link_t child_to_parent_link;
        child_to_parent_link.link_name = relationship.to_parent_link_name();
        child_to_parent_link.source_table = relationship.child().gaia_id();
        child_to_parent_link.target_table = relationship.parent().gaia_id();
        child_to_parent_link.cardinality = catalog::relationship_cardinality_t::one;
        m_arc_values[child_to_parent_arc] = child_to_parent_link;
    }
}

catalog_graph_t::node_t catalog_graph_t::get_node(gaia::common::gaia_id_t table_id)
{
    auto node = m_reverse_node_mapping.find(table_id);
    ASSERT_INVARIANT(node != m_reverse_node_mapping.end(), "The graph must contain the table id");
    return node->second;
}

std::vector<table_path_t> catalog_graph_t::find_paths(common::gaia_id_t from_table, common::gaia_id_t to_table)
{
    node_t source_node = get_node(from_table);
    node_t target_node = get_node(to_table);

    // Uses DFS to find all the paths from the two given tables.
    lemon::Dfs<graph_t> dfs(m_graph);
    dfs.init();

    // Set the root of the graph.
    dfs.addSource(source_node);

    std::vector<path_t> paths;
    path_t curr_path;
    node_t prev_node = source_node;

    while (!dfs.emptyQueue())
    {
        arc_t arc = dfs.processNextArc();

        // Check if we are still following the same path.
        // Consider the following arcs:
        // a->b b->c c->d b->f
        // When we encounter b->f we know we are no longer
        // following the same path because b != d
        if (m_graph.source(arc) == prev_node)
        {
            curr_path.addBack(arc);
            prev_node = m_graph.target(arc);
        }
        else
        {
            // We are no longer following the same path.
            prev_node = m_graph.target(arc);

            if (m_graph.source(arc) == source_node)
            {
                // Started back from the source node,
                // we initialize an empty path
                curr_path = {};
            }
            else
            {
                // We start back somewhere in the middle of the graph
                // we need to get the path from the root.
                curr_path = dfs.path(m_graph.source(arc));
            }

            curr_path.addBack(arc);
        }

        if (m_graph.target(arc) == target_node)
        {
            paths.push_back(curr_path);
        }
    }

    std::vector<table_path_t> return_paths;

    for (path_t path : paths)
    {
        table_path_t table_path;

        for (int i = 0; i < path.length(); i++)
        {
            arc_t arc = path.nth(i);
            table_path.path.push_back(m_arc_values[arc]);
        }

        return_paths.push_back(table_path);
    }

    return return_paths;
}

std::vector<table_path_t> catalog_graph_t::find_shortest_paths(common::gaia_id_t from_table, common::gaia_id_t to_table)
{
    std::vector<table_path_t> all_paths = find_paths(from_table, to_table);

    size_t shortest_length = SIZE_MAX;

    for (table_path_t path : all_paths)
    {
        if (path.path.size() < shortest_length)
        {
            shortest_length = path.path.size();
        }
    }

    std::vector<table_path_t> shortest_paths;

    for (table_path_t path : all_paths)
    {
        if (path.path.size() == shortest_length)
        {
            shortest_paths.push_back(path);
        }
    }

    return shortest_paths;
}

} // namespace translation
} // namespace gaia
