SELECT pg_backend_pid();

CREATE EXTENSION cow_se_fdw;

CREATE SERVER cow_se
    FOREIGN DATA WRAPPER cow_se_fdw
    OPTIONS (data_dir '/gaia-platform/demos/airport/data');

CREATE SCHEMA IF NOT EXISTS airport_demo;

IMPORT FOREIGN SCHEMA airport_demo
    FROM SERVER cow_se INTO airport_demo;

SELECT * FROM information_schema.tables 
WHERE table_schema = 'airport_demo';

SELECT * FROM airport_demo.airports;
SELECT * FROM airport_demo.airlines;
SELECT * FROM airport_demo.routes;
SELECT * FROM airport_demo.event_log;

SELECT table_name, column_name, data_type, is_nullable, column_default
  FROM information_schema.columns
  WHERE table_schema = 'airport_demo'
  ORDER BY table_name, ordinal_position;
