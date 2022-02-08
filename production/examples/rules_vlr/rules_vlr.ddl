---------------------------------------------
-- Copyright (c) 2021 Gaia Platform LLC
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
---------------------------------------------

database rules_vlr

-- College students.
table student (
    id uint32 unique,
    name string,
    -- room_id is optional, so if set to null, that means a student is
    -- not connected to any dorm room.
    room_id uint32 optional,
    -- Create a 1-to-N Value-Linked Relationship between
    -- a dorm room (1) and students (N) that matches room IDs.
    room references dorm_room
        where student.room_id = dorm_room.id
)

-- College dorm rooms where students live.
table dorm_room (
    id uint32 unique,
    name string,
    capacity uint8,
    residents references student[]
)
