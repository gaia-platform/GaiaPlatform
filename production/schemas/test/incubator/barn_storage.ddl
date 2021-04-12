---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

CREATE TABLE IF NOT EXISTS incubator (
      name	 STRING,
      min_temp FLOAT,
      max_temp FLOAT
);

CREATE TABLE IF NOT EXISTS sensor (
      name STRING,
      timestamp UINT64,
      value FLOAT active
);

CREATE TABLE IF NOT EXISTS actuator (
      name STRING,
      timestamp UINT64,
      value FLOAT
);

CREATE RELATIONSHIP IF NOT EXISTS sensor_incubator (
      incubator.sensors -> sensor[],
      sensor.incubator -> incubator
);

CREATE RELATIONSHIP IF NOT EXISTS actuator_incubator (
      incubator.actuators -> actuator[],
      actuator.incubator -> incubator
);
