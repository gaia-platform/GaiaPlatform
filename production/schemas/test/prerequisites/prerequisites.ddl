---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

database prerequisites

table student (
    student_id string,
    surname string,
    age int32,
    total_hours int32,
    gpa float
);

table parents (
    name_father string,
    name_mother string
);

create relationship if not exists student_parents (
    student.parents -> parents,
    parents.student -> student
);

table course (
    course_id string,
    name string,
    hours int32
);

table registration (
    reg_id string,
    status string,
    grade float
);

relationship student_reg (
    student.registrations -> registration[],
    registration.registered_student -> student
);

relationship course_reg (
    course.registrations -> registration[],
    registration.registered_course -> course
);

table prereq (
    prereq_id string,
    min_grade float
);

relationship prereq_course (
    course.required_by -> prereq[],
    prereq.prereq -> course
);

relationship course_prereq (
    course.requires -> prereq[],
    prereq.course -> course
);

table enrollment_log (
    log_student_id string,
    log_surname string,
    log_age int32,
    log_course_id string,
    log_name string,
    log_hours int32,
    log_reg_id string
)
