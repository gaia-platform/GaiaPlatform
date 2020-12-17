create table if not exists person (
      first_name string,
      last_name string,
      birthdate uint32,
      face_signature uint64
);

create table if not exists stranger (
       first_scan uint32,
       face_signature uint64,
       scan_count uint32 active
);

create table if not exists building (
       name string,
       door_closed bool active
);

create table if not exists staff (
      hired_date uint32,
      staff_identity references person
);

create table if not exists parent (
      parent_identity references person
);

create table if not exists student (
      student_identity references person
);

create table if not exists custody (
      custodian references parent,
      child references student,
      relationship uint8
);

create table if not exists face_scan (
       signature uint64,
       time uint32,
       recognized bool,
       location references building,
       owner references person,
       stranger_owner references stranger
);

create table if not exists room (
       name string,
       number string,
       floor string,
       capacity uint32,
       building references building
);

create table if not exists event (
       name string,
       start_time uint32,
       end_time uint32,
       teacher references staff,
       venue references room
);

create table if not exists registration (
       student references student,
       event references event,
       time uint32
);
