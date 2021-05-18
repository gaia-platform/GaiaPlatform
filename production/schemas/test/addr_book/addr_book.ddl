---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

create database if not exists addr_book;

use addr_book;

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

create table if not exists internet_contract (
    provider string,
    contract_id string
);

create relationship if not exists internet_address (
    address.internet_contract -> internet_contract,
    internet_contract.address -> address
);

create table if not exists customer (
    name string,
    sales_by_quarter int32[]
);


create table if not exists A (
    str_val string,
    num_val int32
);

create table if not exists B (
    str_val string,
    num_val int32
);

create relationship if not exists a_b (
   A.b -> B,
   B.a -> A
);


-- create table if not exists C (
--     str_val string,
--     num_val int32
-- );
--
-- create table if not exists D (
--     str_val string,
--     num_val int32
-- );
--
-- create relationship if not exists c_d (
--    C.d_arr -> D[],
--    D.c -> C
-- );
--
-- create relationship if not exists d_c (
--    D.c_arr -> C[],
--    C.d -> D
-- );
