---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

-- Test the scenario where tables are created without a database.

table doctor (
 name string,
 email string unique,
 primary_care_patients references patient[],
 secondary_care_patients references patient[]
)

table patient (
 name string,
 primary_care_doctor_email string,
 secondary_care_doctor_email string,

 primary_care_doctor references doctor
  using primary_care_patients
  where patient.primary_care_doctor_email = doctor.email,

 secondary_care_doctor references doctor
  using secondary_care_patients
  where patient.secondary_care_doctor_email = doctor.email
)
