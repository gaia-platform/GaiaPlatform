---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

DROP DATABASE IF EXISTS hello;

CREATE DATABASE hello;

\c hello;

CREATE EXTENSION gaia_fdw;

CREATE SERVER gaia FOREIGN DATA WRAPPER gaia_fdw;

CREATE SCHEMA hello_fdw;

IMPORT FOREIGN SCHEMA hello_fdw
    FROM SERVER gaia INTO hello_fdw;
