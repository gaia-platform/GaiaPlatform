----------------------------------------------------
-- Copyright (c) Gaia Platform LLC
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
----------------------------------------------------

database direct_access_vlr

-- Floors in a multi-story building.
table floor (
    floor_number int32 unique,
    department string,
    -- Since a Value Linked Relationship will be made from people to a floor,
    -- we also need a reference back from a floor to people.
    people references person[]
)

table person (
    name string,
    floor_number int32,
    -- Create a 1:N VLR between a floor (1) and people (N)
    -- that matches floor numbers.
    current_floor references floor
        where person.floor_number = floor.floor_number
)
