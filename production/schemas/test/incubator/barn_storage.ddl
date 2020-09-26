
CREATE TABLE IF NOT EXISTS incubator (
      name	 STRING,
      min_temp FLOAT32,
      max_temp FLOAT32
);

CREATE TABLE IF NOT EXISTS sensor (
      name STRING,
      timestamp UINT64,
      value FLOAT32 active,
      REFERENCES incubator
);

CREATE TABLE IF NOT EXISTS actuator (
      name STRING,
      timestamp UINT64,
      value FLOAT32,
      REFERENCES incubator
);

