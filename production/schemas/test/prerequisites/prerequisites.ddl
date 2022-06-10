----------------------------------------------------
-- Copyright (c) Gaia Platform Authors
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
----------------------------------------------------

database prerequisites

create table if not exists student (
    student_id string unique,
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

create table if not exists course (
    course_id string unique,
    name string,
    hours int32
);

table registration (
    reg_id string,
    student_id string,
    course_id string,
    status string,
    grade float
);

relationship student_reg (
    student.registrations -> registration[],
    registration.registered_student -> student,
    using registration(student_id), student(student_id)
);

relationship course_reg (
    course.registrations -> registration[],
    registration.registered_course -> course,
    using registration(course_id), course(course_id)
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
);

table major (
    major_name string
);

create relationship if not exists major_courses (
    major.courses -> course[],
    course.course_major -> major
);

table floor (
    num int32 unique,
    department string,
    people references person[]
)

table person (
    person_name string,
    floor_num int32,
    floor references floor
        where person.floor_num = floor.num
)

table state (
    pick bool,
    place bool
)

table pick_action ()

table move_action ()
