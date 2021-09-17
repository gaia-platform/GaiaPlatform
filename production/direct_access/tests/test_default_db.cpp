/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"

#include "gaia/common.hpp"
#include "gaia/direct_access/auto_transaction.hpp"

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia.h"

using namespace gaia::direct_access;

using gaia::common::gaia_id_t;

class default_db_test : public gaia::db::db_catalog_test_base_t
{
protected:
    default_db_test()
        : db_catalog_test_base_t("no_db.ddl"){};
};

TEST_F(default_db_test, direct_access)
{
    // Test direct access APIs working as expected when no database is specified
    // in the DDL. The direct access classes generated from the default database
    // will be under the `gaia` namespace (instead of `gaia::[database]`).
    auto_transaction_t txn;
    gaia_id_t dr_house_id = gaia::doctor_t::insert("Gregory House", "greg.house@ppth.org");
    gaia_id_t dr_wilson_id = gaia::doctor_t::insert("James Wilson", "james.wilson@ppth.org");
    txn.commit();
    gaia_id_t patient_id
        = gaia::patient_t::insert(
            "Rebecca Adler",
            "james.wilson@ppth.org",
            "greg.house@ppth.org");
    txn.commit();
    ASSERT_EQ(gaia::patient_t::get(patient_id).primary_care_doctor().gaia_id(), dr_wilson_id);
    ASSERT_EQ(gaia::patient_t::get(patient_id).secondary_care_doctor().gaia_id(), dr_house_id);
}
