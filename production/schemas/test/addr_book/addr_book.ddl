---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

drop database if exists addr_book;

create database addr_book

create table employee (
    name_first string active,
    name_last string,
    ssn string,
    hire_date int64,
    email string,
    web string,
    manager references employee,
    reportees references employee[],
    addresses references address[],
    phones references phone[],
    internet_contract references internet_contract
)

create table address (
    street string,
    apt_suite string,
    city string,
    state string,
    postal string,
    country string,
    current bool,
    owner references employee,
    phones references phone[],
    internet_contract references internet_contract
)

create table phone (
    phone_number string active,
    type string active,
    primary bool active,
    address references address,
    owner references employee
)

create table internet_contract (
    provider string,
    owner references employee,
    address references address
);

create table customer (
    name string,
    sales_by_quarter int32[]
);
