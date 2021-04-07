CREATE TABLE if not exists animal (
    name STRING,
    breed STRING,
    age UINT64
);

CREATE TABLE if not exists farmer (
    name STRING,
    acreage UINT32
);

CREATE TABLE if not exists crop (
    name STRING
);

CREATE TABLE if not exists incubator (
    name STRING,
    min_temp FLOAT,
    max_temp FLOAT,
    REFERENCES farmer
);

CREATE TABLE if not exists  sensor (
    name STRING,
    timestamp UINT64,
    value FLOAT,
    i_ REFERENCES incubator
);

CREATE TABLE if not exists  actuator (
    name STRING,
    timestamp UINT64,
    value FLOAT,
    i_ REFERENCES incubator
);

CREATE TABLE if not exists raised (
    birthdate STRING,
    REFERENCES animal,
    REFERENCES farmer,
    REFERENCES incubator
);

CREATE TABLE if not exists yield (
    bushels UINT32,
    REFERENCES farmer,
    REFERENCES crop
);

CREATE TABLE if not exists feeding (
    portion UINT32,
    REFERENCES yield,
    REFERENCES animal
);

CREATE TABLE if not exists isolated (
    age UINT32
);
