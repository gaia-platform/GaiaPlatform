---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

DROP TABLE IF EXISTS intermediate_routes;

DROP EXTENSION IF EXISTS gaia_fdw CASCADE;

DROP SCHEMA IF EXISTS airport_demo CASCADE;

DROP INDEX IF EXISTS rawdata_route_uidx;

DROP TABLE IF EXISTS rawdata_airports;

DROP TABLE IF EXISTS rawdata_airlines;

DROP TABLE IF EXISTS rawdata_routes;

CREATE EXTENSION gaia_fdw;

CREATE SERVER gaia FOREIGN DATA WRAPPER gaia_fdw
-- OPTIONS (
--    RESET 'true'
--)
;

CREATE SCHEMA airport_demo;

--IMPORT FOREIGN SCHEMA airport_demo
--FROM
--    SERVER gaia INTO airport_demo;

create foreign table airport_demo.airports(
    gaia_id bigint,
    ap_id int,
    name text,
    city text,
    country text,
    iata char(3),
    icao char(4),
    latitude double precision,
    longitude double precision,
    altitude int,
    timezone float,
    dst char(1),
    tztext text,
    type text,
    source text
) server gaia;

create foreign table airport_demo.airlines(
    gaia_id bigint,
    al_id int,
    name text,
    alias text,
    iata char(3),
    icao char(5),
    callsign text,
    country text,
    active char(1)
) server gaia;

create foreign table airport_demo.routes(
    gaia_id bigint,
--    gaia_src_id bigint,
--    gaia_dst_id bigint,
    airline text,
    al_id int,
    src_ap varchar(4),
    src_ap_id int,
    dst_ap varchar(4),
    dst_ap_id int,
    codeshare char(1),
    stops int,
    equipment text
) server gaia;

-- raw_tables are the csv tables without any gaia info in them.
CREATE TABLE rawdata_airports (
    ap_id int PRIMARY KEY,
    name text,
    city text,
    country text,
    iata char(3) UNIQUE,
    icao char(4) UNIQUE,
    latitude double precision,
    longitude double precision,
    altitude int,
    timezone float,
    dst char(1),
    tztext text,
    type text,
    source text
);

CREATE TABLE rawdata_airlines (
    al_id int PRIMARY KEY,
    name text,
    alias text,
    iata char(3),
    icao char(5),
    callsign text,
    country text,
    active char(1)
);

CREATE TABLE rawdata_routes (
    airline text,
    al_id int,
    src_ap varchar(4),
    src_ap_id int,
    dst_ap varchar(4),
    dst_ap_id int,
    codeshare char(1),
    stops int,
    equipment text
);

CREATE UNIQUE INDEX rawdata_route_uidx ON rawdata_routes (al_id, src_ap_id, dst_ap_id);

-- data is unique in this system airline to 2 airports
-- Loads data into tables from csv files in /data/internal/airport.
-- Tables must already be created.
-- Data comes from openflights.com which has an open database license, free to use; this is a subset of rows focused on SeaTac.

\set airports_file :data_dir '/airports.dat'
COPY rawdata_airports (
    ap_id,
    name,
    city,
    country,
    iata,
    icao,
    latitude,
    longitude,
    altitude,
    timezone,
    dst,
    tztext,
    TYPE,
    source)
FROM
    :'airports_file' DELIMITER ',' csv quote AS '"' NULL AS '\N';

\set airlines_file :data_dir '/airlines.dat'
COPY rawdata_airlines (
    al_id,
    name,
    alias,
    iata,
    icao,
    callsign,
    country,
    active)
FROM
    :'airlines_file' DELIMITER ',' csv quote AS '"' NULL AS '\N';

\set routes_file :data_dir '/routes.dat'
COPY rawdata_routes (
    airline,
    al_id,
    src_ap,
    src_ap_id,
    dst_ap,
    dst_ap_id,
    codeshare,
    stops,
    equipment)
FROM
    :'routes_file' DELIMITER ',' csv quote AS '"' NULL AS '\N';

INSERT INTO airport_demo.airports (
    ap_id,
    name,
    city,
    country,
    iata,
    icao,
    latitude,
    longitude,
    altitude,
    timezone,
    dst,
    tztext,
    TYPE,
    source)
SELECT
    ap_id,
    name,
    city,
    country,
    iata,
    icao,
    latitude,
    longitude,
    altitude,
    timezone,
    dst,
    tztext,
    TYPE,
    source
FROM
    rawdata_airports;

INSERT INTO airport_demo.airlines (
    al_id,
    name,
    alias,
    iata,
    icao,
    callsign,
    country,
    active)
SELECT
    al_id,
    name,
    alias,
    iata,
    icao,
    callsign,
    country,
    active
FROM
    rawdata_airlines;

-- Create a Postgres copy of Gaia airports table.
-- CREATE TABLE airports_copy (
--     gaia_id bigint,
--     ap_id int PRIMARY KEY,
--     name text,
--     city text,
--     country text,
--     iata char(3),
--     icao char(4) UNIQUE,
--     latitude double precision,
--     longitude double precision,
--     altitude int,
--     timezone float,
--     dst char(1),
--     tztext text,
--     type text,
--     source text
-- );

-- INSERT INTO airports_copy (
--     gaia_id,
--     ap_id,
--     name,
--     city,
--     country,
--     iata,
--     icao,
--     latitude,
--     longitude,
--     altitude,
--     timezone,
--     dst,
--     tztext,
--     TYPE,
--     source)
-- SELECT
--     gaia_id,
--     ap_id,
--     name,
--     city,
--     country,
--     iata,
--     icao,
--     latitude,
--     longitude,
--     altitude,
--     timezone,
--     dst,
--     tztext,
--     TYPE,
--     source
-- FROM
--     airport_demo.airports;

-- Create intermediate routes table.
-- CREATE TABLE intermediate_routes (
--     gaia_src_id bigint,
--     gaia_dst_id bigint,
--     airline text,
--     al_id int,
--     src_ap varchar(4),
--     src_ap_id int,
--     dst_ap varchar(4),
--     dst_ap_id int,
--     codeshare char(1),
--     stops int,
--     equipment text
-- );

-- INSERT INTO intermediate_routes (
--     airline,
--     al_id,
--     src_ap,
--     src_ap_id,
--     dst_ap,
--     dst_ap_id,
--     codeshare,
--     stops,
--     equipment)
-- SELECT
--     airline,
--     al_id,
--     src_ap,
--     src_ap_id,
--     dst_ap,
--     dst_ap_id,
--     codeshare,
--     stops,
--     equipment
-- FROM
--     rawdata_routes;

-- Collect the foreign keys from airports_copy table.
-- UPDATE
--     intermediate_routes
-- SET
--     (gaia_src_id) = (
--         SELECT
--             gaia_id
--         FROM
--             airports_copy
--         WHERE
--             airports_copy.ap_id = intermediate_routes.src_ap_id);

-- UPDATE
--     intermediate_routes
-- SET
--     (gaia_dst_id) = (
--         SELECT
--             gaia_id
--         FROM
--             airports_copy
--         WHERE
--             airports_copy.ap_id = intermediate_routes.dst_ap_id);

-- DROP TABLE airports_copy;

-- Remove records that are missing foreign keys.
-- DELETE FROM intermediate_routes
-- WHERE gaia_src_id IS NULL
--     OR gaia_dst_id IS NULL;

-- Finally, we can insert the data into the routes table.
INSERT INTO airport_demo.routes (
--    gaia_src_id,
--    gaia_dst_id,
    airline,
    al_id,
    src_ap,
    src_ap_id,
    dst_ap,
    dst_ap_id,
    codeshare,
    stops,
    equipment)
SELECT
--    gaia_src_id,
--    gaia_dst_id,
    airline,
    al_id,
    src_ap,
    src_ap_id,
    dst_ap,
    dst_ap_id,
    codeshare,
    stops,
    equipment
FROM
--    intermediate_routes;
    rawdata_routes;

--DROP TABLE intermediate_routes;
