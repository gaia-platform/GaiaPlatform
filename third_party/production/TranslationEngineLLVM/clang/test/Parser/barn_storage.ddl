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
    name STRING,
    acres UINT32
);

CREATE TABLE if not exists incubator (
    name STRING,
    min_temp FLOAT,
    max_temp FLOAT
);

create relationship if not exists farmer_incubators (
    farmer.incubators -> incubator[],
    incubator.farmer -> farmer
);

CREATE TABLE if not exists  sensor (
    name STRING,
    timestamp UINT64,
    value FLOAT
);

create relationship if not exists incubator_sensors (
    incubator.sensors -> sensor[],
    sensor.incubator -> incubator
);

CREATE TABLE if not exists  actuator (
    name STRING,
    timestamp UINT64,
    value FLOAT
);

create relationship if not exists incubator_actuators (
    incubator.actuators -> actuator[],
    actuator.incubator -> incubator
);

CREATE TABLE if not exists raised (
    birthdate STRING
);

create relationship if not exists animal_raised (
    animal.raised -> raised[],
    raised.animal -> animal
);

create relationship if not exists farmer_raised (
    farmer.raised -> raised[],
    raised.farmer -> farmer
);

create relationship if not exists incubator_raised (
    incubator.raised -> raised[],
    raised.incubator -> incubator
);

CREATE TABLE if not exists yield (
    bushels UINT32
);

create relationship if not exists farmer_yield (
    farmer.yield -> yield[],
    yield.farmer -> farmer
);

create relationship if not exists crop_yield (
    crop.yield -> yield[],
    yield.crop -> crop
);

CREATE TABLE if not exists feeding (
    portion UINT32
);

create relationship if not exists yield_feeding (
    yield.feeding -> feeding[],
    feeding.yield -> yield
);

create relationship if not exists animal_feeding (
    animal.feeding -> feeding[],
    feeding.animal -> animal
);


CREATE TABLE if not exists isolated (
    age UINT32
);
