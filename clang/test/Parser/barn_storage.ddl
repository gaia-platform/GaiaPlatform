
CREATE TABLE incubator (
      name	 STRING,
      min_temp FLOAT32,
      max_temp FLOAT32
);

CREATE TABLE  sensor (
      incubator_id UINT64,
      name STRING,
      timestamp UINT64,
      value FLOAT32,
      i_ REFERENCES incubator
);

CREATE TABLE  actuator (
      incubator_id UINT64,
      name STRING,
      timestamp UINT64,
      value FLOAT32,
      i_ REFERENCES incubator
);

