----------------------------------------------------
-- Copyright (c) Gaia Platform Authors
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
----------------------------------------------------

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

table table_j1
(
    j2 references table_j2[]
)

table table_j2
(
    j1 references table_j1,
    j3 references table_j3[]
)

table table_j3
(
    j2 references table_j2
)
