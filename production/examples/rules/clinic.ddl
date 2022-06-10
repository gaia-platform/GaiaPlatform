----------------------------------------------------
-- Copyright (c) Gaia Platform Authors
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
----------------------------------------------------

database clinic

table physician
(
    name string,
    is_attending bool,
    residents references resident[]
)

table location (
    id uint32 unique,
    street string,
    city string,
    resident references resident[]
        where location.id = resident.location_id
)

table resident (
    name string,
    location_id uint32,
    height uint8 optional,
    is_intern bool,
    evaluation_results float[],
    physician references physician,
    location references location
)
