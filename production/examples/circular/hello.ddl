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

create relationship if not exists names_greetings_1 (
   names.preferred_greeting -> greetings,
   greetings.name_1 -> names
);

create relationship if not exists names_greetings_M (
   names.greetings -> greetings[],
   greetings.name_M -> names
);