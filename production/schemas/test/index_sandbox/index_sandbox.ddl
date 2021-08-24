---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

CREATE DATABASE IF NOT EXISTS index_sandbox;

USE index_sandbox;

CREATE TABLE if not exists sandbox (
    str STRING,
    f FLOAT,
    i INT32
);

CREATE INDEX str_idx ON sandbox(str);
CREATE HASH INDEX str_hash_idx ON sandbox(str);

CREATE INDEX int_idx ON sandbox(i);
CREATE HASH INDEX int_hash_idx ON sandbox(i);

CREATE INDEX float_idx ON sandbox(f);
CREATE HASH INDEX fload_has_idx ON sandbox(f);
