---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

DROP DATABASE IF EXISTS test_array;

CREATE DATABASE test_array;

\c test_array;

CREATE EXTENSION gaia_fdw;

CREATE SERVER gaia FOREIGN DATA WRAPPER gaia_fdw;

CREATE SCHEMA test_fdw;

IMPORT FOREIGN SCHEMA test_fdw
FROM
   SERVER gaia INTO test_fdw;
