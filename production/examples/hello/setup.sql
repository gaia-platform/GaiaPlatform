---------------------------------------------
-- Copyright (c) 2021 Gaia Platform LLC
--
-- Use of this source code is governed by an MIT-style
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
---------------------------------------------

DROP DATABASE IF EXISTS hello;

CREATE DATABASE hello;

\c hello;

CREATE EXTENSION gaia_fdw;

CREATE SERVER gaia FOREIGN DATA WRAPPER gaia_fdw;

CREATE SCHEMA hello_fdw;

IMPORT FOREIGN SCHEMA hello_fdw
    FROM SERVER gaia INTO hello_fdw;
