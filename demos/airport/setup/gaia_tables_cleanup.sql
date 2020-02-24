---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

drop foreign table airports;
drop foreign table airlines;
drop foreign table routes;

drop server gaia_server_apq;
drop server gaia_server_alq;
drop server gaia_server_rq;

drop extension multicorn cascade;

