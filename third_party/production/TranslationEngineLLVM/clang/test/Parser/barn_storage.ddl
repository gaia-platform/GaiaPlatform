create database if not exists incubator;

USE incubator;

create table if not exists animal (
    name string,
    breed string,
    age uint64
);

create table if not exists farmer (
    name string,
    acreage uint32
);

create table if not exists crop (
    name string,
    acres uint32
);

create table if not exists incubator (
    name string,
    min_temp float,
    max_temp float
);

create relationship if not exists farmer_incubators (
    farmer.incubators -> incubator[],
    incubator.farmer -> farmer
);

create relationship if not exists farmer_condos (
    farmer.condos -> incubator[],
    incubator.landlord -> farmer
);

create table if not exists  sensor (
    name string,
    timestamp uint64,
    value float
);

create relationship if not exists incubator_sensors (
    incubator.sensors -> sensor[],
    sensor.incubator -> incubator
);

create table if not exists  actuator (
    name string,
    timestamp uint64,
    value float
);

create relationship if not exists incubator_actuators (
    incubator.actuators -> actuator[],
    actuator.incubator -> incubator
);

create table if not exists raised (
    birthdate string
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

create table if not exists yield (
    bushels uint32
);

create relationship if not exists farmer_yield (
    farmer.yield -> yield[],
    yield.farmer -> farmer
);

create relationship if not exists crop_yield (
    crop.yield -> yield[],
    yield.crop -> crop
);

create relationship if not exists crop_animal (
    crop.animal -> animal[],
    animal.crop -> crop
);

create table if not exists feeding (
    portion uint32
);

create relationship if not exists yield_feeding (
    yield.feeding -> feeding[],
    feeding.yield -> yield
);

create relationship if not exists animal_feeding (
    animal.feeding -> feeding[],
    feeding.animal -> animal
);

create table if not exists isolated (
    age uint32
);

create table if not exists hyperconnected (
    val uint32
);

create table if not exists target (
    val uint32
);

create relationship if not exists hyperconnected_target_1 (
    hyperconnected.link1 -> target[],
    target.back_link1 -> hyperconnected
);

create relationship if not exists hyperconnected_target_2 (
    hyperconnected.link2 -> target[],
    target.back_link2 -> hyperconnected
);
