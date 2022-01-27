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
    student_id uint32 unique,
    name string,
    room_location string,
    room references dorm_room
        where student.room_location = dorm_room.location

)

-- College dorm rooms where students live.
table dorm_room (
    location string unique,
    capacity uint8,
    residents references student[]
)
