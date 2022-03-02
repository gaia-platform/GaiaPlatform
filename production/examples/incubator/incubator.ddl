----------------------------------------------------
-- Copyright (c) Gaia Platform LLC
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
----------------------------------------------------

create database if not exists incubator;

use incubator;

create table if not exists incubator (
    name string unique,
    is_on bool,
    min_temp float,
    max_temp float,
    callback references callback[] where incubator.name = callback.name
)

create table if not exists callback (
    name string,
    instance uint64,
    incubator references incubator
)

create table if not exists sensor (
    name string,
    timestamp uint64,
    value float
)

create table if not exists actuator (
    name string,
    timestamp uint64,
    value float
)

create relationship if not exists incubator_sensors (
    incubator.sensors -> sensor[],
    sensor.incubator -> incubator
)

create relationship if not exists incubator_actuators (
    incubator.actuators -> actuator[],
    actuator.incubator -> incubator
);
