---------------------------------------------
-- Copyright (c) 2021 Gaia Platform LLC
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
---------------------------------------------

database vlr_example

table floor (
    num int32 unique,
    department string,
    people references person[]
)

table person (
    name string,
    floor_num int32,
    -- Create a 1-to-N Value-Linked Relationship between a floor (1) and a person (N)
    -- that matches floor numbers.
    floor references floor
        where person.floor_num = floor.num
)
