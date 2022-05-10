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
    description string,
    -- Since a Value Linked Relationship will be made from elevators to a floor,
    -- we also need a reference back from a floor to elevators.
    elevators references elevator[]
)

-- Elevators that travel up and down floors.
table elevator (
    elevator_id uint32,
    floor_number int32,
    -- Create a 1:N VLR between a floor (1) and elevators (N)
    -- that matches floor numbers.
    current_floor references floor
        where elevator.floor_number = floor.floor_number
)
