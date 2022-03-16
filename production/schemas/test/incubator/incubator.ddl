---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

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

table state (
    pick bool,
    place bool
)

table pick_action ()

table move_action ()
