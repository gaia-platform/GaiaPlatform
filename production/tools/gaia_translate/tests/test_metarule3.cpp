/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <string>

#include <gtest/gtest.h>

#include "gaia/rules/rules.hpp"

#include "gaia_internal/db/db_catalog_test_base.hpp"
#include "gaia_internal/rules/rules_test_helpers.hpp"

#include "gaia_prerequisites.h"
#include "test_rulesets.hpp"

using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::prerequisites;
using namespace gaia::rules;

extern int g_total_hours;
extern int g_high_grade_hours;
extern int g_low_grade_hours;

class tools__gaia_translate__metarule3__test : public db_catalog_test_base_t
{
public:
    tools__gaia_translate__metarule3__test()
        : db_catalog_test_base_t("prerequisites.ddl", true, true, true)
    {
    }

protected:
    void SetUp() override
    {
        db_catalog_test_base_t::SetUp();
        gaia::rules::initialize_rules_engine();
    }

    void TearDown() override
    {
        db_catalog_test_base_t::TearDown();
        unsubscribe_rules();
        gaia::rules::shutdown_rules_engine();
    }
};

TEST_F(tools__gaia_translate__metarule3__test, basic)
{
    gaia::db::begin_transaction();

    auto student = student_t::get(student_t::insert_row("stu001", "Richard", 45, 4, 3.0));

    course_t::insert_row("cou001", "math101", 3);
    course_t::insert_row("cou002", "math201", 4);
    course_t::insert_row("cou003", "eng101", 3);
    course_t::insert_row("cou004", "sci101", 3);
    course_t::insert_row("cou005", "math301", 5);

    registration_t::insert_row("reg001", "stu001", "cou002", c_status_pending, c_grade_none);
    registration_t::insert_row("reg002", "stu001", "cou004", c_status_eligible, c_grade_c);
    registration_t::insert_row("reg003", "stu001", "cou001", c_status_eligible, c_grade_b);
    registration_t::insert_row("reg004", "stu001", "cou003", c_status_eligible, c_grade_a);
    registration_t::insert_row("reg006", "stu001", "cou005", c_status_pending, c_grade_d);

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    ASSERT_EQ(g_total_hours, 18);
    ASSERT_EQ(g_high_grade_hours, 6);
    ASSERT_EQ(g_low_grade_hours, 12);

    gaia::db::begin_transaction();
    auto sw = student.writer();
    sw.age = 30;
    sw.update_row();
    gaia::db::commit_transaction();

    ASSERT_EQ(g_high_grade_hours, 6);
    ASSERT_EQ(g_low_grade_hours, 12);
}
