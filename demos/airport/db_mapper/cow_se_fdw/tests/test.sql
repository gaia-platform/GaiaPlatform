CREATE EXTENSION cow_se_fdw;

CREATE SERVER cow_se
	FOREIGN DATA WRAPPER cow_se_fdw;

CREATE FOREIGN TABLE films (
    code        char(5) NOT NULL,
    title       varchar(40) NOT NULL,
    did         integer NOT NULL,
    date_prod   date,
    kind        varchar(10),
    len         interval hour to minute
)
SERVER cow_se;

INSERT INTO films (code, title, did, date_prod, kind)
    VALUES ('T_601', 'Yojimbo', 106, '1961-06-16', 'Drama');

SELECT * FROM films;
