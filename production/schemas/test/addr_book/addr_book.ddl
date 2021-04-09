---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

create table if not exists employee (
    name_first string active,
    name_last string,
    ssn string,
    hire_date int64,
    email string,
    web string
);

create relationship if not exists manager_reportees (
    employee.reportees -> employee[],
    employee.manager -> employee
);

create table if not exists address (
    street string,
    apt_suite string,
    city string,
    state string,
    postal string,
    country string,
    current bool
);

create relationship if not exists address_owner (
    employee.addresses -> address[],
    address.owner -> employee
);

create table if not exists phone (
    phone_number string active,
    type string active,
    primary bool active
);

create relationship if not exists address_phones (
    address.phones -> phone[],
    phone.address -> address
);

create relationship if not exists phone_owner (
    employee.phones -> phone[],
    phone.owner -> employee
);

create table if not exists customer (
    name string,
    sales_by_quarter int32[]
);
