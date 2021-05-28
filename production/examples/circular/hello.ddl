---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

create database if not exists hello;

use hello;

create table if not exists names
(
    name string
);

create table if not exists greetings
(
    greeting string
);

create relationship if not exists names_greetings (
   names.greetings -> greetings,
   greetings.names -> names
);