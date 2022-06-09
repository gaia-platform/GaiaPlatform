----------------------------------------------------
-- Copyright (c) Gaia Platform Authors
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
----------------------------------------------------

create database if not exists mink;

use mink;

create table if not exists incubator (
      name string,
      is_on bool,
      min_temp float,
      max_temp float
);

create table if not exists sensor (
      name string,
      timestamp uint64,
      value float
);

create table if not exists actuator (
      name string,
      timestamp uint64,
      value float
);

create relationship if not exists incubator_sensors (
    incubator.sensors -> sensor[],
    sensor.incubator -> incubator
);

create relationship if not exists incubator_actuators (
    incubator.actuators -> actuator[],
    actuator.incubator -> incubator
);
