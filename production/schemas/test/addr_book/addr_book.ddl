---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

database addr_book

table employee (
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

table address (
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

table phone (
    phone_number string active,
    type string active,
    primary bool active,
    address references address,
    owner references employee
)

table internet_contract (
    provider string,
    owner references employee,
    address references address
)

table customer (
    name string,
    sales_by_quarter int32[]
)

table company(
    company_name string,
    clients references client[]
)

table client (
    client_name string,
    sales int32[],
    company references company
)
