----------------------------------------------------
-- Copyright (c) Gaia Platform Authors
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
----------------------------------------------------

CREATE DATABASE IF NOT EXISTS index_sandbox;

USE index_sandbox;

CREATE TABLE if not exists sandbox (
    str STRING,
    f FLOAT,
    i INT32,
    optional_i INT32 optional
);

CREATE TABLE if not exists empty (
    i INT32
);

CREATE INDEX str_idx ON sandbox(str);
CREATE HASH INDEX str_hash_idx ON sandbox(str);

CREATE INDEX int_idx ON sandbox(i);
CREATE HASH INDEX int_hash_idx ON sandbox(i);

CREATE INDEX float_idx ON sandbox(f);
CREATE HASH INDEX float_hash_idx ON sandbox(f);

CREATE INDEX empty_idx ON empty(i);
CREATE HASH INDEX empty_hash_idx ON empty(i);

CREATE INDEX opt_int_idx ON sandbox(optional_i);
CREATE HASH INDEX opt_int_hash_idx ON sandbox(optional_i);
