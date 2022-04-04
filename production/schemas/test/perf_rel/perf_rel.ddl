---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

database perf_rel

table table_parent
(
    children references table_child[]
)

table table_child
(
    parent references table_parent
)

table table_parent_vlr
(
    id uint64 unique,
    children_vlr references table_child_vlr[]
)

table table_child_vlr
(
    parent_id uint64,
    parent_vlr references table_parent_vlr
        where table_child_vlr.parent_id = table_parent_vlr.id
)
