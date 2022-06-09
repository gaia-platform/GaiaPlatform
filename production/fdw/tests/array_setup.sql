----------------------------------------------------
-- Copyright (c) Gaia Platform Authors
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
----------------------------------------------------

DROP DATABASE IF EXISTS test_array;

CREATE DATABASE test_array;

\c test_array;

CREATE EXTENSION gaia_fdw;

CREATE SERVER gaia FOREIGN DATA WRAPPER gaia_fdw;

CREATE SCHEMA test_fdw;

IMPORT FOREIGN SCHEMA test_fdw
FROM
   SERVER gaia INTO test_fdw;
