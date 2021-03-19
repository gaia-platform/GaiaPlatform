---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

create table if not exists incubator (
      name string,
      is_on bool,
      min_temp float,
      max_temp float
);

create table if not exists sensor (
      name string,
      timestamp uint64,
      value float,
      references incubator
);

create table if not exists actuator (
      name string,
      timestamp uint64,
      value float,
      references incubator
);
