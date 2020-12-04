
CREATE TABLE if not exists incubator (
      name	 STRING,
      min_temp FLOAT,
      max_temp FLOAT
);

CREATE TABLE if not exists  sensor (
      incubator_id UINT64,
      name STRING,
      timestamp UINT64,
      value FLOAT,
      i_ REFERENCES incubator
);

CREATE TABLE if not exists  actuator (
      incubator_id UINT64,
      name STRING,
      timestamp UINT64,
      value FLOAT,
      i_ REFERENCES incubator
);

