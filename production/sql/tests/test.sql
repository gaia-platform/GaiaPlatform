---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

\c airport;

-- Find all airports which connect flights from airports SEA to OTP.
SELECT DISTINCT
    a.name
FROM
    airport_fdw.routes r1,
    airport_fdw.routes r2,
    airport_fdw.airports a
WHERE
    r1.src_ap = 'SEA'
    AND r1.gaia_dst_id = r2.gaia_src_id
    AND r2.dst_ap = 'OTP'
    AND a.gaia_id = r1.gaia_dst_id
ORDER BY
    a.name;

-- Find all cities containing airports which connect flights from airports SEA to OTP, with the second flight on airline RO.
SELECT DISTINCT
    a.city
FROM
    airport_fdw.routes r1,
    airport_fdw.routes r2,
    airport_fdw.airports a
WHERE
    r1.src_ap = 'SEA'
    AND r1.gaia_dst_id = r2.gaia_src_id
    AND r2.dst_ap = 'OTP'
    AND r2.airline = 'RO'
    AND r1.gaia_dst_id = a.gaia_id
ORDER BY
    a.city;

-- Find all routes connecting airports SEA and OTP over 2 hops.
SELECT
    r1.src_ap,
    r1.airline,
    r1.dst_ap,
    r2.airline,
    r2.dst_ap
FROM
    airport_fdw.routes r1,
    airport_fdw.routes r2
WHERE
    r1.src_ap = 'SEA'
    AND r1.gaia_dst_id = r2.gaia_src_id
    AND r2.dst_ap = 'OTP'
ORDER BY
    r1.src_ap,
    r1.airline,
    r1.dst_ap,
    r2.airline,
    r2.dst_ap;

-- Find all routes connecting airports SEA and OTP over 2 hops, that are not on the airline RO.
SELECT
    r1.src_ap,
    r1.airline,
    r1.dst_ap,
    r2.airline,
    r2.dst_ap
FROM
    airport_fdw.routes r1,
    airport_fdw.routes r2
WHERE
    r1.src_ap = 'SEA'
    AND r1.gaia_dst_id = r2.gaia_src_id
    AND r2.dst_ap = 'OTP'
    AND r1.airline <> 'RO'
    AND r2.airline <> 'RO'
ORDER BY
    r1.src_ap,
    r1.airline,
    r1.dst_ap,
    r2.airline,
    r2.dst_ap;

-- Array insertion
INSERT INTO airport_fdw.passengers(name, miles_flown_by_quarter)
VALUES('Jane Doe', ARRAY[1000,2000, 3000, 4000, 5000, 6000]);

INSERT INTO airport_fdw.passengers(name, miles_flown_by_quarter)
VALUES('Joe Public', ARRAY[500, 500, 1000, 1500, 2500, 4000]);

-- Array selection
SELECT
    name, miles_flown_by_quarter
FROM
    airport_fdw.passengers
ORDER BY
    name;

-- Array element selection
SELECT
    name, miles_flown_by_quarter[4]
FROM
    airport_fdw.passengers
ORDER BY
    name;

-- Array update
UPDATE airport_fdw.passengers
SET miles_flown_by_quarter[6] = 2000
WHERE name = 'Jane Doe';

SELECT miles_flown_by_quarter[6]
FROM airport_fdw.passengers
WHERE name = 'Jane Doe';

-- Array deletion
DELETE FROM airport_fdw.passengers WHERE name = 'Jane Doe';
SELECT name FROM airport_fdw.passengers;
