---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

drop database if exists airportdemo_validation;
create database airportdemo_validation;
\c airportdemo_validation;

\i ../setup/gaia_tables_setup.sql;

-- attempt to insert different data types.
insert into airports(gaia_id) values(666);
insert into airports(name) values('Seattle');
insert into airports(iata) values('sea');
insert into airports(latitude) values(95.7);
insert into airports(timezone) values(8.5);

insert into airports(ap_id, gaia_id) values(10, 10);
insert into airports(ap_id, gaia_id) values(11, 11);
insert into airlines(al_id, gaia_id) values(20, 20);
insert into airlines(al_id, gaia_id) values(21, 21);
insert into airlines(al_id, gaia_id) values(90, 90);

insert into routes(airline, al_id, gaia_src_id, gaia_dst_id) values ('alaska', 20, 10, 11);
select al_id + 10000 from airlines;

select * from airports limit 10;
select * from airlines limit 10;
select * from routes limit 10;

\i ../setup/gaia_tables_cleanup.sql;

\c postgres;

drop database if exists airportdemo_validation;
