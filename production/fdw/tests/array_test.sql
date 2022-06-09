----------------------------------------------------
-- Copyright (c) Gaia Platform Authors
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
----------------------------------------------------

\c test_array;

-- Array insertion
INSERT INTO test_fdw.passengers(name, miles_flown_by_quarter)
VALUES('Jane Doe', ARRAY[1000,2000, 3000, 4000, 5000, 6000]);

INSERT INTO test_fdw.passengers(name, miles_flown_by_quarter)
VALUES('Joe Public', ARRAY[500, 500, 1000, 1500, 2500, 4000]);

-- Array selection
SELECT
    name, miles_flown_by_quarter
FROM
    test_fdw.passengers
ORDER BY
    name;

-- Array element selection
SELECT
    name, miles_flown_by_quarter[4]
FROM
    test_fdw.passengers
ORDER BY
    name;

-- Array update
UPDATE test_fdw.passengers
SET miles_flown_by_quarter[6] = 2000
WHERE name = 'Jane Doe';

SELECT miles_flown_by_quarter[6]
FROM test_fdw.passengers
WHERE name = 'Jane Doe';

-- Array deletion
DELETE FROM test_fdw.passengers WHERE name = 'Jane Doe';
SELECT name FROM test_fdw.passengers;
