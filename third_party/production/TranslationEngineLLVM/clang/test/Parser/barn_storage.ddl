
CREATE TABLE if not exists incubator (
      name STRING active,
      min_temp FLOAT active,
      max_temp FLOAT active
);

CREATE TABLE if not exists  sensor (
      name STRING active,
      timestamp UINT64 ,
      value FLOAT active,
      i_ REFERENCES incubator
);

CREATE TABLE if not exists  actuator (
      name STRING active,
      timestamp UINT64 ,
      value FLOAT active,
      i_ REFERENCES incubator
);


CREATE TABLE if not exists animal (
   name string active,
   age UINT64 active
);
