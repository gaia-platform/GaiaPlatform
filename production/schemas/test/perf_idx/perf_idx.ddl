---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

database perf_idx

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
