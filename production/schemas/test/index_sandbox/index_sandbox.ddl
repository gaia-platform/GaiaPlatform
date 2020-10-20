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

--CREATE INDEX str_idx ON sandbox(str);
--CREATE HASH INDEX str_hash_idx ON sandbox(str);
