----------------------------------------------------
-- Copyright (c) Gaia Platform Authors
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
----------------------------------------------------

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
