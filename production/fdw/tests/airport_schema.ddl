----------------------------------------------------
-- Copyright (c) Gaia Platform LLC
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
----------------------------------------------------

create table if not exists airlines
(
    al_id int32,
    name string,
    alias string,
    iata string,
    icao string,
    callsign string,
    country string,
    is_active string
);

create table if not exists airports
(
    ap_id int32,
    name string,
    city string,
    country string,
    iata string,
    icao string,
    latitude double,
    longitude double,
    altitude int16,
    timezone float,
    dst string,
    tztext string,
    type string,
    source string
);

create table if not exists routes
(
    airline string,
    al_id int32,
    src_ap string,
    src_ap_id int32,
    dst_ap string,
    dst_ap_id int32,
    codeshare string,
    stops int16,
    equipment string
);

create relationship if not exists routes_src
(
    airports.routes_from -> routes[],
    routes.gaia_src_id -> airports
);

create relationship if not exists routes_dst
(
    airports.routes_to -> routes[],
    routes.gaia_dst_id -> airports
);
