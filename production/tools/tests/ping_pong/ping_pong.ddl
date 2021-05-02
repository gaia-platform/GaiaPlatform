---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

create database if not exists ping_pong;

use ping_pong;

create table if not exists ping_pong (
    status string active
);
