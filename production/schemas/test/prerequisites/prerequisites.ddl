---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

drop database if exists prerequisites;

create database prerequisites

create table student (
    student_id string,
    surname string,
    age int32,
    total_hours int32,
    gpa float,
    parents references parents,
    registrations references registration[]
)

create table parents (
    name_father string,
    name_mother string,
    student references student
)

create table course (
    course_id string,
    name string,
    hours int32
)

create table registration (
    reg_id string,
    status string,
    grade string,
    registered_student references student
)

create relationship course_reg (
    course.registrations -> registration[],
    registration.registered_course -> course
)

create table prereq (
    prereq_id string,
    min_grade string
)

create relationship prereq_course (
    course.required_by -> prereq[],
    prereq.prereq -> course
)

create relationship course_prereq (
    course.requires -> prereq[],
    prereq.course -> course
)

create table if not exists enrollment_log (
    log_student_id string,
    log_surname string,
    log_age int32,
    log_course_id string,
    log_name string,
    log_hours int32,
    log_reg_id string
);
