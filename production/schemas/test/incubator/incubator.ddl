---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

DROP DATABASE IF EXISTS barn_storage;

CREATE DATABASE barn_storage

CREATE TABLE incubator (
      name	 STRING,
      min_temp FLOAT,
      max_temp FLOAT,
      sensors REFERENCES sensor[],
      actuators REFERENCES actuator[]
)

CREATE TABLE sensor (
      name STRING,
      timestamp UINT64,
      value FLOAT active,
      incubator REFERENCES incubator
)

CREATE TABLE actuator (
      name STRING,
      timestamp UINT64,
      value FLOAT,
      incubator REFERENCES incubator
);
