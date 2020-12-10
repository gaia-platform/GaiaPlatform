create table if not exists campus (
      name : string,
      in_emergency : bool active
);

create table if not exists building (
      name : string,
      BuildingCampus references campus
);

create table if not exists person (
      name : string,
      is_threat : uint64 active,
      location : string active,
      PersonCampus references campus
);

create table if not exists role (
      name : string,
      RolePerson references person
);

create table if not exists locations (
      name : string,
      LocationsCampus references campus
);

