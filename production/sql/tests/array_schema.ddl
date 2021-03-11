---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

create table if not exists passengers
(
    name string,
    miles_flown_by_quarter int32[]
);
