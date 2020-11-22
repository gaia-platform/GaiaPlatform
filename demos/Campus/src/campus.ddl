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
      location : string active,
      is_threat : bool active,
      references campus
);

create table if not exists locations (
      name : string,
      references campus
);

