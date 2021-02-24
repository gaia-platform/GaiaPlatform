---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

\c airport;

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
