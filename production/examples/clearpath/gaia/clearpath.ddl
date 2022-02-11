----------------------------------------------------
-- Copyright (c) Gaia Platform LLC
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
----------------------------------------------------

database clearpath

table graph
(
    uuid string unique, -- DEFAULT "aaaaaaaa-0000-0000-0000-000000000000"
    vertices references vertex[],
    edges references edge[],
    point_edges references point_edge[]
)

table vertex
(
    id int64 unique,
    type int64,
    pose uint8[],
    deprecation_count int64,
    reference int64,
    session int64,
    data uint8[],
    pose_x double,
    pose_y double,
    stamp_sec int64,
    stamp_nsec int64,
    level int64,
    compression string,
    source string,
    confidence double,

    -- Relationships
    graph_uuid string,
    graph references graph
        using vertices
        where vertex.graph_uuid = graph.uuid,

    in_edges references edge[],
    out_edges references edge[],

    in_p_edges references point_edge[],
    in_pt_edges references point_edge[],
    out_p_edges references point_edge[]
)

table edge
(
    id int64 unique,

    gen_id string,
    pose uint8[],
    confidence double,
    robust int64,

    -- Relationships
    graph_uuid string,
    graph references graph
        where edge.graph_uuid = graph.uuid,

    dest_id int64,
    dest references vertex
        using in_edges
        where edge.dest_id = vertex.id,

    src_id int64,
    src references vertex
        using out_edges
        where edge.src_id = vertex.id
)

table point_edge
(
    id int64 unique,

    confidence double,
    robust int64,

    -- Relationships
    graph_uuid string,
    graph references graph
        where point_edge.graph_uuid = graph.uuid,

    dest_id int64,
    dest references vertex
        using in_p_edges
        where point_edge.dest_id = vertex.id,

    dest_pt_id int64,
    dest_pt references vertex
        using in_pt_edges
        where point_edge.dest_pt_id = vertex.id,

    src_id int64,
    src references vertex
        using out_p_edges
        where point_edge.src_id = vertex.id
)
