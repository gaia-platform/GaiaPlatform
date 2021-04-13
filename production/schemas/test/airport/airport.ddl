---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

create table if not exists airport
(
    name string,
    city string,
    iata string
);

create table if not exists flight
(
    number int32,
    miles_flown_by_quarter int32[]
);

create table if not exists segment
(
    miles int32,
    status int8,
    luggage_weight int32,
    references flight,
    src references airport,
    dst references airport
);
