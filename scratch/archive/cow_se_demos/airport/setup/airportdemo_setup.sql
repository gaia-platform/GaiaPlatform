---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

drop database if exists airportdemo;
create database airportdemo;
\c airportdemo;

-- raw_tables are the csv tables without any gaia info in them.
create table rawdata_airports(
    ap_id int primary key, name text, city text, country text, iata char(3) unique, icao char(4) unique,
    latitude double precision, longitude double precision, altitude int, timezone float,
    dst char(1), tztext text, type text, source text);

create table rawdata_airlines(
    al_id int primary key, name text, alias text, iata char(3), icao char(4),
    callsign text, country text, active char(1));

create table rawdata_routes(
    airline text, al_id int, src_ap varchar(4), src_ap_id int, dst_ap varchar(4), dst_ap_id int,
    codeshare char(1), stops int, equipment text);

create unique index rawdata_route_uidx on rawdata_routes(al_id, src_ap_id, dst_ap_id); -- data is unique in this system airline to 2 airports

-- Loads data into tables from csv files in /tmp.
-- Tables must already be created.
-- Data comes from openflights.com which has an open database license, free to use; this is a subset of rows focused on SeaTac.

-- Copy data to /tmp:
--      cp airports.dat /tmp
--      cp airlines.dat /tmp
--      cp routes.dat /tmp

copy rawdata_airports(
        ap_id, name, city, country, iata, icao, latitude, longitude, altitude, timezone,
        dst, tztext, type, source)
    from '/tmp/airports.dat'
        delimiter ',' csv quote as '"' null as '\N';

copy rawdata_airlines(
        al_id, name, alias, iata, icao, callsign, country, active)
    from '/tmp/airlines.dat'
        delimiter ',' csv quote as '"' null as '\N';

copy rawdata_routes(
        airline, al_id, src_ap, src_ap_id, dst_ap, dst_ap_id, codeshare, stops, equipment)
    from '/tmp/routes.dat'
        delimiter ',' csv quote as '"' null as '\N';

