---------------------------------------------
-- Copyright (c) 2021 Gaia Platform LLC
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
---------------------------------------------

database vlr_example

-- Levels in a multi-story building.
table level (
    num int32 unique,
    department string,
    -- Since a Value-Linked Relationship will be made from people to a level,
    -- we also need a reference back from a level to people.
    people references person[]
)

table person (
    name string,
    level_num int32,
    -- Create a 1-to-N VLR between a level (1) and a person (N)
    -- that matches level numbers.
    current_level references level
        where person.level_num = level.num
)
