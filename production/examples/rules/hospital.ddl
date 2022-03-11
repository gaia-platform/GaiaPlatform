----------------------------------------------------
-- Copyright (c) Gaia Platform LLC
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
----------------------------------------------------

database hospital

table doctor (
    name string,
    is_active bool,
    patients references patient[]
)

table patient (
    name string,
    address_id uint32 unique,
    height uint8 optional,
    is_active bool,
    analysis_results float[],
    doctor references doctor,
    address references address[]
        where patient.address_id = address.id
)

table address (
    id uint32 unique,
    street string,
    city string,
    patient references patient
)

-- TODO (dax): add an unrelated table
