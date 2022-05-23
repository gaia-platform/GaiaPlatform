----------------------------------------------------
-- Copyright (c) Gaia Platform LLC
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
----------------------------------------------------

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
