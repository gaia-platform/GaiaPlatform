---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

create extension multicorn;

create server gaia_server_apq foreign data wrapper multicorn options(wrapper 'db_mapper.AirportQuery');
create server gaia_server_alq foreign data wrapper multicorn options(wrapper 'db_mapper.AirlineQuery');
create server gaia_server_rq foreign data wrapper multicorn options(wrapper 'db_mapper.RouteQuery');

create foreign table airports(
    gaia_id bigint,
    ap_id int, name text, city text, country text, iata char(3), icao char(4),
    latitude double precision, longitude double precision, altitude int,
    timezone float, dst char(1), tztext text, type text, source text)
    server gaia_server_apq;

create foreign table airlines(
    gaia_id bigint,
    al_id int,
    name text, alias text, iata char(3), icao char(4),
    callsign text, country text, active char(1))
    server gaia_server_alq;

create foreign table routes(
    gaia_id bigint, gaia_src_id bigint, gaia_dst_id bigint,
    airline text, al_id int,
    src_ap varchar(4), src_ap_id int, dst_ap varchar(4), dst_ap_id int,
    codeshare char(1), stops int, equipment text)
    server gaia_server_rq;

