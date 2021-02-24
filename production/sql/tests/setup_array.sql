---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

DROP DATABASE IF EXISTS airport;

CREATE DATABASE airport;

\c airport;

CREATE EXTENSION gaia_fdw;

CREATE SERVER gaia FOREIGN DATA WRAPPER gaia_fdw;

CREATE SCHEMA airport_fdw;

IMPORT FOREIGN SCHEMA airport_fdw
FROM
   SERVER gaia INTO airport_fdw;
