create table if not exists incubator (
      name : string,
      is_on :  bool active,
      min_temp : float active,
      max_temp : float active
);

create table if not exists sensor (
      name : string,
      timestamp : uint64,
      value : float active,
      references incubator
);

create table if not exists actuator (
      name string,
      timestamp uint64,
      value float active,
      references incubator
);
