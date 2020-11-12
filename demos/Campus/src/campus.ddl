create table incubator (
      name string,
      is_on bool active,
      min_temp float32 active,
      max_temp float32 active
);

create table sensor (
      name string,
      timestamp uint64,
      value float32 active,
      references incubator
);

create table actuator (
      name string,
      timestamp uint64,
      value float32 active,
      references incubator
);
