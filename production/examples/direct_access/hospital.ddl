----------------------------------------------------
-- Copyright (c) Gaia Platform Authors
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
----------------------------------------------------

database hospital

table doctor (
    name string,
    patients references patient[]
)

table patient (
    name string,
    height uint8 optional,
    is_active bool,
    analysis_results float[],
    doctor references doctor,
    address references address
)

table address (
    street string,
    city string,
    patient references patient
)
