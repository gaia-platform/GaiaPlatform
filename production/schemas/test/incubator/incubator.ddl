----------------------------------------------------
-- Copyright (c) Gaia Platform Authors
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
----------------------------------------------------

DATABASE barn_storage

TABLE incubator (
      name STRING,
      min_temp FLOAT,
      max_temp FLOAT,
      sensors REFERENCES sensor[],
      actuators REFERENCES actuator[]
)

TABLE sensor (
      name STRING,
      timestamp UINT64,
      value FLOAT active,
      incubator REFERENCES incubator
)

TABLE actuator (
      name STRING,
      timestamp UINT64,
      value FLOAT,
      incubator REFERENCES incubator
)
