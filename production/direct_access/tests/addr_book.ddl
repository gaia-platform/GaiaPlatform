create table employee (
    name_first: string,
    name_last: string,
    ssn: string,
    hire_date: int64,
    email: string,
    web: string,
    manages references employee
);

create table address (
    street: string,
    apt_suite: string,
    city: string,
    state: string,
    postal: string,
    country: string,
    current: bool,
    addresses references employee
);

create table phone (
    phone_number: string,
    type: string,
    primary: bool,
    phones references address
);
