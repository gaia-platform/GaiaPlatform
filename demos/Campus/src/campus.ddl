create table if not exists campus (
      name : string,
      in_emergency : bool active
);

create table if not exists building (
      name : string,
      references campus
);

create table if not exists person (
      name : string,
      is_threat : uint64 active,
      location : string active,
      references campus
);

create table if not exists locations (
      name : string,
      references campus
);

