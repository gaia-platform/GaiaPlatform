---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

database hospital

table doctor (
    name string,
    email string unique,
    patients references patient[]
)

table patient (
    name string,
    height uint8,
    is_active bool,
    doctor_email string,
    analysis_results float[],
    doctor references doctor,
    address references address
)

table address (
    street string,
    city string,
    patient references patient
)
