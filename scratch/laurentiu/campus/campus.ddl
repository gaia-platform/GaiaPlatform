create table if not exists persons
(
    first_name string,
    last_name string,
    birth_date string,
    contact_info string,
    face_signature string,
    alert_on_sight bool,
    alert_description string
);

create table if not exists staff
(
    staff_person references persons,
    hiring_date string,
    title string
);

create table if not exists guardians
(
    guardian_person references persons,
    relationship string
);

create table if not exists students
(
    student_person references persons,
    primary_guardian references guardians,
    secondary_guardian references guardians,
    tertiary_guardian references guardians
);

create table if not exists buildings
(
    name string,
    code string
);

create table if not exists rooms
(
    name string,
    code string,
    purpose string,
    room_building references buildings,
    floor int16,
    capacity int16
);

create table if not exists events
(
    name string,
    topic string,
    agenda string,
    date string,
    begin_time int16,
    end_time int16,
    event_room references rooms
);

create table if not exists event_coordination
(
    coordinator references persons,
    coordinated_event references events
);

create table if not exists event_attendance
(
    attendee references persons,
    attended_event references events
);

create table if not exists face_scanning
(
    face_scanning_building references buildings,
    date string,
    time int16,
    face_signature string active
);
