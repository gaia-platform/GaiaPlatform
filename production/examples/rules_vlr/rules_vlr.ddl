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
    room_id uint32,
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
