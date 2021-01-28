---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

\c airportdemo;

\i gaia_tables_setup.sql;

insert into airports(
        ap_id, name, city, country, iata, icao, latitude, longitude, altitude, timezone,
        dst, tztext, type, source)
    select
        ap_id, name, city, country, iata, icao, latitude, longitude, altitude, timezone,
        dst, tztext, type, source
    from rawdata_airports;

insert into airlines(
        al_id, name, alias, iata, icao, callsign, country, active)
    select
        al_id, name, alias, iata, icao, callsign, country, active
    from rawdata_airlines;

-- Create a Postgres copy of Gaia airports table.
create table airports_copy(
    gaia_id bigint,
    ap_id int primary key, name text, city text, country text, iata char(3) unique, icao char(4) unique,
    latitude double precision, longitude double precision, altitude int, timezone float,
    dst char(1), tztext text, type text, source text);

insert into airports_copy(
        gaia_id, ap_id, name, city, country, iata, icao, latitude, longitude, altitude, timezone,
        dst, tztext, type, source)
    select
        gaia_id, ap_id, name, city, country, iata, icao, latitude, longitude, altitude, timezone,
        dst, tztext, type, source
    from airports;

-- Create intermediate routes table.
create table intermediate_routes(
    gaia_src_id bigint, gaia_dst_id bigint,
    airline text, al_id int, src_ap varchar(4), src_ap_id int, dst_ap varchar(4), dst_ap_id int,
    codeshare char(1), stops int, equipment text);

insert into intermediate_routes(
        airline, al_id, src_ap, src_ap_id, dst_ap, dst_ap_id, codeshare, stops, equipment)
    select
        airline, al_id, src_ap, src_ap_id, dst_ap, dst_ap_id, codeshare, stops, equipment
    from rawdata_routes;

-- Collect the foreign keys from airports_copy table.
update intermediate_routes
    set (gaia_src_id)
        = (select gaia_id from airports_copy where airports_copy.ap_id = intermediate_routes.src_ap_id);

update intermediate_routes
    set (gaia_dst_id)
        = (select gaia_id from airports_copy where airports_copy.ap_id = intermediate_routes.dst_ap_id);

drop table airports_copy;

-- Remove records that are missing foreign keys.
delete from intermediate_routes where gaia_src_id is null or gaia_dst_id is null;

-- Finally, we can insert the data into the routes table.
insert into routes(
        gaia_src_id, gaia_dst_id, airline, al_id, src_ap, src_ap_id, dst_ap, dst_ap_id, codeshare, stops, equipment)
    select
        gaia_src_id, gaia_dst_id, airline, al_id, src_ap, src_ap_id, dst_ap, dst_ap_id, codeshare, stops, equipment
    from intermediate_routes;

drop table intermediate_routes;

