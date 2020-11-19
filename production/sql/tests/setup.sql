---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

DROP DATABASE IF EXISTS airport;

CREATE DATABASE airport;

\c airport;

CREATE EXTENSION gaia_fdw;

CREATE SERVER gaia FOREIGN DATA WRAPPER gaia_fdw
-- OPTIONS (
--    RESET 'true'
--)
;

CREATE SCHEMA airport_fdw;

IMPORT FOREIGN SCHEMA airport_fdw
FROM
   SERVER gaia INTO airport_fdw;

-- The rawdata tables are used to import the data from files.
CREATE TABLE rawdata_airlines (
    al_id int PRIMARY KEY,
    name text,
    alias text,
    iata char(3),
    icao char(5),
    callsign text,
    country text,
    is_active char(1)
);

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

-- Load airport data into rawdata tables.
\set airlines_file :data_dir '/airlines.dat'
COPY rawdata_airlines (
    al_id,
    name,
    alias,
    iata,
    icao,
    callsign,
    country,
    is_active)
FROM
    :'airlines_file' DELIMITER ',' csv quote AS '"' NULL AS '\N';

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

-- Now insert the data into the Gaia tables.
INSERT INTO airport_fdw.airlines (
    al_id,
    name,
    alias,
    iata,
    icao,
    callsign,
    country,
    is_active)
SELECT
    al_id,
    name,
    alias,
    iata,
    icao,
    callsign,
    country,
    is_active
FROM
    rawdata_airlines;

INSERT INTO airport_fdw.airports (
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

-- Create a Postgres copy of Gaia airports table.
CREATE TABLE airports_copy (
    gaia_id bigint,
    ap_id int PRIMARY KEY,
    name text,
    city text,
    country text,
    iata char(3),
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

INSERT INTO airports_copy (
    gaia_id,
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
    gaia_id,
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
    airport_fdw.airports;

-- Create intermediate routes table.
CREATE TABLE intermediate_routes (
    gaia_src_id bigint,
    gaia_dst_id bigint,
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

INSERT INTO intermediate_routes (
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
    rawdata_routes;

-- Collect the foreign keys from airports_copy table.
UPDATE
    intermediate_routes
SET
    (gaia_src_id) = (
        SELECT
            gaia_id
        FROM
            airports_copy
        WHERE
            airports_copy.ap_id = intermediate_routes.src_ap_id);

UPDATE
    intermediate_routes
SET
    (gaia_dst_id) = (
        SELECT
            gaia_id
        FROM
            airports_copy
        WHERE
            airports_copy.ap_id = intermediate_routes.dst_ap_id);

-- Remove records that are missing foreign keys.
DELETE FROM intermediate_routes
WHERE gaia_src_id IS NULL
    OR gaia_dst_id IS NULL;

DROP TABLE airports_copy;

-- Finally, we can insert the data into the routes table.
INSERT INTO airport_fdw.routes (
    gaia_src_id,
    gaia_dst_id,
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
    gaia_src_id,
    gaia_dst_id,
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
    intermediate_routes;
    -- rawdata_routes;

DROP TABLE intermediate_routes;

-- Alternative approach: update airport_fdw.routes data in-place.
-- This approach exercises the UPDATE path rather than the INSERT path.
--
-- Collect the foreign keys from airports_copy table.
-- UPDATE
--     airport_fdw.routes
-- SET
--     (gaia_src_id) = (
--         SELECT
--             gaia_id
--         FROM
--             airport_fdw.airports
--         WHERE
--             airport_fdw.airports.ap_id = airport_fdw.routes.src_ap_id);

-- UPDATE
--     airport_fdw.routes
-- SET
--     (gaia_dst_id) = (
--         SELECT
--             gaia_id
--         FROM
--             airport_fdw.airports
--         WHERE
--             airport_fdw.airports.ap_id = airport_fdw.routes.dst_ap_id);

-- Remove records that are missing foreign keys.
-- DELETE FROM airport_fdw.routes
-- WHERE gaia_src_id IS NULL
--     OR gaia_dst_id IS NULL;
