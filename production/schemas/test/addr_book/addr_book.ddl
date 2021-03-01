create table if not exists employee (
    name_first string active,
    name_last string,
    ssn string,
    hire_date int64,
    email string,
    web string,
    manages references employee
);

create table if not exists address (
    street string,
    apt_suite string,
    city string,
    state string,
    postal string,
    country string,
    current bool,
    addressee references employee
);

create table if not exists phone (
    phone_number string active,
    type string active,
    primary bool active,
    references address,
    references employee
);

create table if not exists customer (
    name string,
    sales_by_quarter int32[]
);
