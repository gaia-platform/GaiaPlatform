/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia/rules/rules.hpp"

#include "gaia_internal/db/db_catalog_test_base.hpp"
#include "gaia_internal/rules/rules_test_helpers.hpp"

#include "gaia_prerequisites.h"
#include "test_rulesets.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::rules;
using namespace gaia::prerequisites;
using namespace gaia::prerequisites::registration_expr;

class test_connect_disconnect : public db_catalog_test_base_t
{
public:
    test_connect_disconnect()
        : db_catalog_test_base_t("prerequisites.ddl"){};

protected:
    void SetUp() override
    {
        db_catalog_test_base_t::SetUp();
        gaia::rules::initialize_rules_engine();
        gaia::rules::unsubscribe_rules();
    }

    void TearDown() override
    {
        db_catalog_test_base_t::TearDown();
        unsubscribe_rules();
        gaia::rules::shutdown_rules_engine();
    }
};

TEST_F(test_connect_disconnect, test_connect_1_n)
{
    gaia::rules::subscribe_ruleset("test_connect_1_n");

    gaia::db::begin_transaction();

    // The rule will create a connection between the student and these registrations.
    student_waynetype student_1 = student_waynetype::get(student_waynetype::insert_row("stu001", "Richard", 45, 4, 3.0));
    registration_waynetype::insert_row("reg006", "stu001", nullptr, c_status_eligible, c_grade_c);
    registration_waynetype::insert_row("reg007", "stu001", nullptr, c_status_eligible, c_grade_c);

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    vector expected_regs = {"reg001", "reg002", "reg003", "reg004", "reg005", "reg006", "reg007"};

    gaia::db::begin_transaction();

    ASSERT_EQ(expected_regs.size(), student_1.registrations().size());

    for (const auto& expected_reg : expected_regs)
    {
        ASSERT_EQ(student_1.registrations().where(reg_id == expected_reg).size(), 1);
    }

    gaia::db::commit_transaction();
}

TEST_F(test_connect_disconnect, test_connect_1_1)
{
    gaia::rules::subscribe_ruleset("test_connect_1_1");

    gaia::db::begin_transaction();

    student_waynetype student_1 = student_waynetype::get(student_waynetype::insert_row("stu001", "Richard", 45, 4, 3.0));
    student_waynetype student_2 = student_waynetype::get(student_waynetype::insert_row("stu002", "Hazel", 45, 4, 3.0));

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    gaia::db::begin_transaction();

    ASSERT_STREQ(student_1.parents().name_father(), "Claudio");
    ASSERT_STREQ(student_1.parents().name_mother(), "Patrizia");

    ASSERT_STREQ(student_2.parents().name_father(), "John");
    ASSERT_STREQ(student_2.parents().name_mother(), "Jane");

    gaia::db::commit_transaction();
}

TEST_F(test_connect_disconnect, test_disconnect_1_n)
{
    gaia::rules::subscribe_ruleset("test_disconnect_1_n");

    gaia::db::begin_transaction();
    student_waynetype student_1 = student_waynetype::get(student_waynetype::insert_row("stu001", "Richard", 45, 4, 3.0));
    registration_waynetype::insert_row("reg001", "stu001", nullptr, c_status_eligible, c_grade_c);
    registration_waynetype::insert_row("reg002", "stu001", nullptr, c_status_eligible, c_grade_c);
    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    gaia::db::begin_transaction();
    student_waynetype student_2 = student_waynetype::get(student_waynetype::insert_row("stu002", "Hazel", 45, 4, 3.0));
    registration_waynetype::insert_row("reg003", "stu002", nullptr, c_status_eligible, c_grade_c);
    registration_waynetype::insert_row("reg004", "stu002", nullptr, c_status_eligible, c_grade_c);
    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    gaia::db::begin_transaction();
    course_waynetype course_1 = course_waynetype::get(course_waynetype::insert_row("cou001", "math101", 8));
    registration_waynetype::insert_row("reg005", nullptr, "cou001", c_status_eligible, c_grade_c);
    registration_waynetype::insert_row("reg006", nullptr, "cou001", c_status_eligible, c_grade_c);
    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    gaia::db::begin_transaction();
    course_waynetype course_2 = course_waynetype::get(course_waynetype::insert_row("cou002", "math102", 8));
    registration_waynetype::insert_row("reg007", nullptr, "cou002", c_status_eligible, c_grade_c);
    registration_waynetype::insert_row("reg008", nullptr, "cou002", c_status_eligible, c_grade_c);
    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    gaia::db::begin_transaction();
    ASSERT_EQ(0, student_1.registrations().size());
    //    ASSERT_EQ(0, student_2.registrations().size());
    ASSERT_EQ(0, course_1.registrations().size());
    //    ASSERT_EQ(0, course_2.registrations().size());
    gaia::db::commit_transaction();
}

TEST_F(test_connect_disconnect, test_disconnect_1_1)
{
    gaia::rules::subscribe_ruleset("test_disconnect_1_1");

    gaia::db::begin_transaction();
    gaia_id_t parents_1 = parents_waynetype::insert_row("Winnie", "Pontus");
    student_waynetype student_1 = student_waynetype::get(student_waynetype::insert_row("stu001", "Richard", 45, 4, 3.0));
    student_1.parents().connect(parents_1);
    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    gaia::db::begin_transaction();
    ASSERT_FALSE(student_1.parents());
    gaia::db::commit_transaction();
}

TEST_F(test_connect_disconnect, disconnect_delete)
{
    gaia::rules::subscribe_ruleset("test_disconnect_delete");

    gaia::db::begin_transaction();
    student_waynetype student_1 = student_waynetype::get(student_waynetype::insert_row("stu001", "Richard", 45, 4, 3.0));
    registration_waynetype::insert_row("reg001", "stu001", nullptr, c_status_eligible, c_grade_c);
    registration_waynetype::insert_row("reg002", "stu001", nullptr, c_status_eligible, c_grade_c);
    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    gaia::db::begin_transaction();
    int count = student_1.registrations().size();
    gaia::db::commit_transaction();
    ASSERT_EQ(count, 0);
}
