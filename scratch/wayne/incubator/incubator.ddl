create table if not exists incubator (
      name string,
      is_on bool active,
      start_time int32,
      min_temp float active,
      max_temp float active
);

create table if not exists sensor (
      name string,
      timestamp int32,
      value float active,
      references incubator
);

create table if not exists actuator (
      name string,
      timestamp int32,
      value float active,
      references incubator
);
