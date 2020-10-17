
CREATE TABLE IF NOT EXISTS incubator (
      name	 STRING,
      min_temp FLOAT,
      max_temp FLOAT
);

CREATE TABLE IF NOT EXISTS sensor (
      name STRING,
      timestamp UINT64,
      value FLOAT active,
      REFERENCES incubator
);

CREATE TABLE IF NOT EXISTS actuator (
      name STRING,
      timestamp UINT64,
      value FLOAT,
      REFERENCES incubator
);

