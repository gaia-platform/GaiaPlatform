
CREATE TABLE if not exists incubator (
      name	 STRING,
      min_temp FLOAT32,
      max_temp FLOAT32
);

CREATE TABLE  if not exists sensor (
      name STRING,
      timestamp UINT64,
      value FLOAT32 active,
      REFERENCES incubator
);

CREATE TABLE  if not exists actuator (
      name STRING,
      timestamp UINT64,
      value FLOAT32,
      REFERENCES incubator
);

