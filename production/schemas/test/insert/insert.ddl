---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

database insert_sandbox

table simple_table_t
(
    uint64_field uint64
)

table simple_table_2
(
    uint64_field uint64,
    string_field string,
    vector uint64[]
)

table simple_table_3
(
    uint64_field_1 uint64,
    uint64_field_2 uint64,
    uint64_field_3 uint64,
    uint64_field_4 uint64,
    string_field_1 string,
    string_field_2 string,
    string_field_3 string,
    string_field_4 string
)

table unique_index_table
(
    uint64_field uint64 unique
)

table hash_index_table
(
    uint64_field uint64
)
create hash index int_hash_idx ON hash_index_table(uint64_field)

table range_index_table
(
    uint64_field uint64
)
create range index int_range_idx ON range_index_table(uint64_field)

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
