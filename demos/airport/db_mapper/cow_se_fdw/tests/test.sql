SELECT pg_backend_pid();
SET log_min_messages = 'DEBUG1';
DROP EXTENSION cow_se_fdw CASCADE;

CREATE EXTENSION cow_se_fdw;

CREATE SERVER cow_se
    FOREIGN DATA WRAPPER cow_se_fdw
    OPTIONS (data_dir '/gaia-platform/demos/airport/data');

CREATE SCHEMA IF NOT EXISTS airport_demo;

IMPORT FOREIGN SCHEMA airport_demo
    FROM SERVER cow_se INTO airport_demo;

SELECT * FROM information_schema.tables 
WHERE table_schema = 'airport_demo';

SELECT table_name, column_name, data_type, is_nullable, column_default
  FROM information_schema.columns
  WHERE table_schema = 'airport_demo'
  ORDER BY table_name, ordinal_position;

SELECT * FROM airport_demo.airports;
SELECT * FROM airport_demo.airlines;
SELECT * FROM airport_demo.routes;
SELECT * FROM airport_demo.event_log;

SELECT FROM airport_demo.airlines a WHERE a.gaia_id=1;

DELETE FROM airport_demo.airlines a WHERE a.gaia_id=1;

INSERT INTO airport_demo.airlines (gaia_id, al_id, name, alias, iata, icao, callsign, country, active)
VALUES (1, 3, '1Time Airline', '', '1T', 'RNX', 'NEXTIME', 'South Africa', 'Y');

SELECT FROM airport_demo.airlines a WHERE a.gaia_id=1;
