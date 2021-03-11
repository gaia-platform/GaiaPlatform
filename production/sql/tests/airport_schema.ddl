---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

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
    equipment string,
    gaia_src_id references airports,
    gaia_dst_id references airports
);
