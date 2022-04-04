---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

database perf_basic

table simple_table
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
