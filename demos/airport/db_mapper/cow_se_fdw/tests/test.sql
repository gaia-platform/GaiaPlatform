select pg_backend_pid();

CREATE EXTENSION cow_se_fdw;

CREATE SERVER cow_se
    FOREIGN DATA WRAPPER cow_se_fdw
    OPTIONS (data_dir '/gaia-platform/demos/airport/data');

CREATE SCHEMA IF NOT EXISTS airport_demo;

IMPORT FOREIGN SCHEMA airport_demo
    FROM SERVER cow_se INTO airport_demo;

SELECT * FROM information_schema.tables 
WHERE table_schema = 'airport_demo';

select * from airport_demo.airports;
select * from airport_demo.airlines;
select * from airport_demo.routes;
