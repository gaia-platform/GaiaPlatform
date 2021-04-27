---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

CREATE DATABASE IF NOT EXISTS hello;

USE hello;

create table if not exists names
(
    name string
);

create table if not exists greetings
(
    greeting string
);
