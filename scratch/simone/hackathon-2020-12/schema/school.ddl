create table if not exists persons
(
    first_name: string,
    last_name: string,
    birth_date: uint64,
    face_signature: uint64,
    person_type: uint8
);

create table if not exists parents
(
    parent_person references persons,
    parent_type: string
);

create table if not exists students
(
    student_person references persons,
    telephone: string
);

create table if not exists family
(
    student references students,
    father references parents,
    mother references parents
);

create table if not exists staff
(
    person references persons,
    hired_date: uint64
);

create table if not exists buildings
(
    building_name: string,
    camera_id: uint64,
    door_closed: bool active
);

create table if not exists rooms
(
    room_number: uint64,
    room_name: string,
    floor_number: uint64,
    capacity: uint64,
    building references buildings
);

create table if not exists face_scan_log
(
    scan_signature: uint64 active,
    scan_date: uint64 active,
    person_type: uint8 active,
    singature_id references persons,
    building_log references buildings
);

create table if not exists events
(
    name: string,
    start_date: uint64,
    end_time: uint64,
    event_actually_enrolled: bool,
    enrollment: uint64,
    teacher references staff,
    room references rooms
);

create table if not exists registration
(
    registration_date: uint64,
    reg_student references students,
    event references events
);

create table if not exists strangers
(
    scan_count: uint64 active,
    face_scan references face_scan_log
)
