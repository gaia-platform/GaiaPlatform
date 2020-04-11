select pg_backend_pid();

CREATE EXTENSION cow_se_fdw;

CREATE SERVER cow_se
    FOREIGN DATA WRAPPER cow_se_fdw
    OPTIONS (data_dir '/gaia-platform/demos/airport/data');

-- CREATE FOREIGN TABLE films (
--     code        char(5) NOT NULL,
--     title       varchar(40) NOT NULL,
--     did         integer NOT NULL,
--     date_prod   date,
--     kind        varchar(10),
--     len         interval hour to minute
-- )
-- SERVER cow_se;

-- INSERT INTO films (code, title, did, date_prod, kind)
--     VALUES ('T_601', 'Yojimbo', 106, '1961-06-16', 'Drama');

-- SELECT * FROM films;

CREATE SCHEMA IF NOT EXISTS airport_demo;

IMPORT FOREIGN SCHEMA airport_demo
    FROM SERVER cow_se INTO airport_demo;

-- IMPORT FOREIGN SCHEMA airport_demo LIMIT TO (airports, airlines)
--     FROM SERVER cow_se INTO airport_demo;

-- select * from pg_tables where schemaname='airport_demo';

SELECT * FROM information_schema.tables 
WHERE table_schema = 'airport_demo';

select * from airport_demo.airports;
select * from airport_demo.airlines;
select * from airport_demo.routes;
