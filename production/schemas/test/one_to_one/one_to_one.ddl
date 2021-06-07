---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

create database if not exists one_to_one;

use one_to_one;

create table if not exists person (
    name_first string active,
    name_last string
);

-- Self one-to-one relationship
create relationship if not exists wife_husband (
    person.husband -> person,
    person.wife -> person
);

create table if not exists employee (
    company string
);

create relationship if not exists person_employee (
    person.employee -> employee,
    employee.person -> person
);

create table if not exists student (
    school string
);

create relationship if not exists person_student (
    person.student -> student,
    student.person -> person
);
