
CREATE TABLE incubator (
      name	 STRING,
      min_temp FLOAT32,
      max_temp FLOAT32
);

CREATE TABLE  sensor (
      name STRING,
      timestamp UINT64,
      value FLOAT32 active,
      REFERENCES incubator
);

CREATE TABLE  actuator (
      name STRING,
      timestamp UINT64,
      value FLOAT32,
      REFERENCES incubator
);

