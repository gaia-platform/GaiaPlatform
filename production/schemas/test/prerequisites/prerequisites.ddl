---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

create database if not exists prerequisites;

use prerequisites;

create table if not exists student (
    student_id string unique,
    surname string,
    age int32,
    total_hours int32,
    gpa float
);

create table if not exists parents (
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

create table if not exists registration (
    reg_id string,
    student_id string,
    course_id string,
    status string,
    grade string
);

create relationship if not exists student_reg (
    student.registrations -> registration[],
    registration.registered_student -> student,
    using registration(student_id), student(student_id)
);

create relationship if not exists course_reg (
    course.registrations -> registration[],
    registration.registered_course -> course,
    using registration(course_id), course(course_id)
);

create table if not exists prereq (
    prereq_id string,
    min_grade string
);

create relationship if not exists prereq_course (
    course.required_by -> prereq[],
    prereq.prereq -> course
);

create relationship if not exists course_prereq (
    course.requires -> prereq[],
    prereq.course -> course
);

create table if not exists enrollment_log (
    log_student_id string,
    log_surname string,
    log_age int32,
    log_course_id string,
    log_name string,
    log_hours int32,
    log_reg_id string
);
