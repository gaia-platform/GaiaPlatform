/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <atomic>

#include <gtest/gtest.h>

#include "gaia/rules/rules.hpp"

#include "gaia_internal/db/db_catalog_test_base.hpp"
#include "gaia_internal/rules/rules_test_helpers.hpp"

#include "gaia_prerequisites.h"
#include "test_rulesets.hpp"

using namespace std;
using namespace gaia::prerequisites;
using namespace gaia::prerequisites::student_expr;
using namespace gaia::prerequisites::registration_expr;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::rules;

extern bool g_oninsert_called;
extern bool g_oninsert2_called;
extern bool g_oninsert3_called;
extern bool g_onchange_called;
extern bool g_onchange2_called;
extern bool g_onupdate_called;
extern bool g_onupdate2_called;
extern bool g_onupdate3_called;
extern bool g_onupdate4_called;
extern test_error_result_t g_oninsert_result;
extern test_error_result_t g_oninsert2_result;
extern test_error_result_t g_oninsert3_result;
extern test_error_result_t g_onchange_result;
extern test_error_result_t g_onchange2_result;
extern test_error_result_t g_onupdate_result;
extern test_error_result_t g_onupdate2_result;
extern test_error_result_t g_onupdate3_result;
extern test_error_result_t g_onupdate4_result;
extern int32_t g_oninsert_value;
extern int32_t g_oninsert2_value;
extern int32_t g_oninsert3_value;
extern int32_t g_onupdate3_value;
extern string g_string_value;

extern std::atomic<int32_t> g_onupdate_value;
extern std::atomic<int32_t> g_insert_count;

student_t student_1;
student_t student_2;
student_t student_3;
student_t student_4;
student_t student_5;

course_t course_1;
course_t course_2;
course_t course_3;
course_t course_4;
course_t course_5;

registration_t reg_1;
registration_t reg_2;
registration_t reg_3;
registration_t reg_4;
registration_t reg_5;
registration_t reg_6;
registration_t reg_7;
registration_t reg_8;
registration_t reg_9;
registration_t reg_A;
registration_t reg_B;
registration_t reg_C;
registration_t reg_D;
registration_t reg_E;
registration_t reg_F;
registration_t reg_G;

prereq_t prereq_1;
prereq_t prereq_2;
prereq_t prereq_3;
prereq_t prereq_4;

class tools__gaia_translate__insert_delete : public db_catalog_test_base_t
{
public:
    tools__gaia_translate__insert_delete()
        : db_catalog_test_base_t("prerequisites.ddl", true, true, true)
    {
    }

protected:
    void SetUp() override
    {
        db_catalog_test_base_t::SetUp();
        gaia::rules::initialize_rules_engine();
        gaia::rules::unsubscribe_rules();
        g_oninsert_called = false;
        g_oninsert2_called = false;
        g_oninsert3_called = false;
        g_onchange_called = false;
        g_onchange2_called = false;
        g_onupdate_called = false;
        g_onupdate2_called = false;
        g_onupdate3_called = false;
        g_onupdate4_called = false;
        g_oninsert_result = test_error_result_t::e_none;
        g_oninsert2_result = test_error_result_t::e_none;
        g_oninsert3_result = test_error_result_t::e_none;
        g_onchange_result = test_error_result_t::e_none;
        g_onchange2_result = test_error_result_t::e_none;
        g_onupdate_result = test_error_result_t::e_none;
        g_onupdate2_result = test_error_result_t::e_none;
        g_onupdate3_result = test_error_result_t::e_none;
        g_onupdate4_result = test_error_result_t::e_none;
        g_onupdate_value = 0;
        g_insert_count = 0;
    }

    void TearDown() override
    {
        db_catalog_test_base_t::TearDown();
        unsubscribe_rules();
        gaia::rules::shutdown_rules_engine();
    }

    void populate_db()
    {
        /*
        gaia::db::begin_transaction();

        // These must be DAC objects. They have insert() methods.
        student_1 = student_t::get(student_t::insert_row("stu001", "Richard", 45, 4, 3.0));
        student_2 = student_t::get(student_t::insert_row("stu002", "Russell", 32, 4, 3.0));
        student_3 = student_t::get(student_t::insert_row("stu003", "Reuben", 26, 4, 3.0));
        student_4 = student_t::get(student_t::insert_row("stu004", "Rachael", 51, 4, 3.0));
        student_5 = student_t::get(student_t::insert_row("stu005", "Renee", 65, 4, 3.0));

        // These must be DAC objects. They have insert() methods.
        course_1 = course_t::get(course_t::insert_row("cou001", "math101", 3));
        course_2 = course_t::get(course_t::insert_row("cou002", "math201", 4));
        course_3 = course_t::get(course_t::insert_row("cou003", "eng101", 3));
        course_4 = course_t::get(course_t::insert_row("cou004", "sci101", 3));
        course_5 = course_t::get(course_t::insert_row("cou005", "math301", 5));

        reg_1 = registration_t::get(registration_t::insert_row("reg001", c_status_pending, c_grade_none));
        // These are gaia_id_t.
        auto reg_2 = registration_t::insert_row("reg002", c_status_eligible, c_grade_c);
        auto reg_3 = registration_t::insert_row("reg003", c_status_eligible, c_grade_b);
        auto reg_4 = registration_t::insert_row("reg004", c_status_eligible, c_grade_c);
        auto reg_5 = registration_t::insert_row("reg005", c_status_eligible, c_grade_d);
        auto reg_6 = registration_t::insert_row("reg006", c_status_pending, c_grade_none);
        auto reg_7 = registration_t::insert_row("reg007", c_status_eligible, c_grade_c);
        auto reg_8 = registration_t::insert_row("reg008", c_status_eligible, c_grade_b);
        auto reg_9 = registration_t::insert_row("reg009", c_status_eligible, c_grade_b);
        auto reg_A = registration_t::insert_row("reg00A", c_status_eligible, c_grade_a);
        auto reg_B = registration_t::insert_row("reg00B", c_status_eligible, c_grade_b);
        auto reg_C = registration_t::insert_row("reg00C", c_status_eligible, c_grade_b);
        auto reg_D = registration_t::insert_row("reg00D", c_status_pending, c_grade_none);
        auto reg_E = registration_t::insert_row("reg00E", c_status_eligible, c_grade_c);
        auto reg_F = registration_t::insert_row("reg00F", c_status_eligible, c_grade_a);
        auto reg_G = registration_t::insert_row("reg00G", c_status_eligible, c_grade_b);

        // These are gaia_id_t.
        prereq_1 = prereq_t::get(prereq_t::insert_row("pre001", c_grade_c));
        prereq_2 = prereq_t::get(prereq_t::insert_row("pre002", c_grade_d));
        prereq_3 = prereq_t::get(prereq_t::insert_row("pre003", c_grade_c));
        prereq_4 = prereq_t::get(prereq_t::insert_row("pre004", c_grade_c));

        student_1.registrations().insert(reg_1);
        student_1.registrations().insert(reg_2);
        student_2.registrations().insert(reg_3);
        student_2.registrations().insert(reg_4);
        student_2.registrations().insert(reg_5);
        student_3.registrations().insert(reg_6);
        student_3.registrations().insert(reg_7);
        student_3.registrations().insert(reg_8);
        student_3.registrations().insert(reg_9);
        student_3.registrations().insert(reg_A);
        student_4.registrations().insert(reg_B);
        student_4.registrations().insert(reg_C);
        student_5.registrations().insert(reg_D);
        student_5.registrations().insert(reg_E);
        student_5.registrations().insert(reg_F);
        student_5.registrations().insert(reg_G);

        course_1.registrations().insert(reg_3);
        course_1.registrations().insert(reg_8);
        course_1.registrations().insert(reg_C);
        course_1.registrations().insert(reg_G);

        course_2.registrations().insert(reg_1);
        course_2.registrations().insert(reg_7);
        course_2.registrations().insert(reg_D);

        course_3.registrations().insert(reg_4);
        course_3.registrations().insert(reg_9);
        course_3.registrations().insert(reg_E);

        course_4.registrations().insert(reg_2);
        course_4.registrations().insert(reg_5);
        course_4.registrations().insert(reg_A);
        course_4.registrations().insert(reg_B);
        course_4.registrations().insert(reg_F);

        course_5.registrations().insert(reg_6);

        course_2.requires().insert(prereq_1);
        course_1.required_by().insert(prereq_1);

        course_2.requires().insert(prereq_2);
        course_3.required_by().insert(prereq_2);

        course_2.requires().insert(prereq_3);
        course_4.required_by().insert(prereq_3);

        course_5.requires().insert(prereq_4);
        course_2.required_by().insert(prereq_4);

        gaia::db::commit_transaction();
        */
    }
};

// TESTCASE: Create rows procedurally, delete them in the rule.
TEST_F(tools__gaia_translate__insert_delete, implicit_delete)
{
    gaia::db::begin_transaction();
    auto student_1 = student_t::get(student_t::insert_row("stu001", "Richard", 45, 4, 3.0));
    registration_t::insert_row("reg001", nullptr, nullptr, c_status_pending, c_grade_none);
    registration_t::insert_row("reg002", nullptr, nullptr, c_status_eligible, c_grade_c);
    registration_t::insert_row("reg003", nullptr, nullptr, c_status_eligible, c_grade_b);
    registration_t::insert_row("reg004", nullptr, nullptr, c_status_eligible, c_grade_c);
    gaia::db::commit_transaction();

    // Use the rules for insert & delete.
    gaia::rules::subscribe_ruleset("test_insert_delete");

    // Fire on_update(S:student).
    gaia::db::begin_transaction();
    auto sw = student_1.writer();
    sw.age = 46;
    sw.update_row();
    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    // Expected value is number of registrations deleted
    EXPECT_EQ(g_onupdate_value, 4) << "Incorrect count of deleted registrations";
}

// TESTCASE: Generate database within rules
// GAIAPLAT-1250 (fixed)
TEST_F(tools__gaia_translate__insert_delete, build_database)
{
    // Use the rules for insert & delete.
    gaia::rules::subscribe_ruleset("test_insert_delete_2");

    gaia::db::begin_transaction();
    enrollment_log_t::insert_row("stu001", "Wayne", 67, "cou001", "math101", 3, "reg001");
    enrollment_log_t::insert_row("stu002", "William", 23, "cou002", "csci101", 5, "reg002");
    enrollment_log_t::insert_row("stu003", "Ward", 56, "cou003", "math201", 3, "reg003");
    enrollment_log_t::insert_row("stu004", "Walter", 65, "cou004", "engl101", 4, "reg004");
    enrollment_log_t::insert_row("stu005", "Warren", 67, "cou005", "engl201", 3, "reg005");
    enrollment_log_t::insert_row("stu006", "Waldo", 37, "cou006", "sosc101", 5, "reg006");
    enrollment_log_t::insert_row("stu007", "Wilma", 44, "cou007", "sosc201", 5, "reg007");
    enrollment_log_t::insert_row("stu008", "Wesley", 12, "cou086", "csci302", 5, "reg008");
    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    int row_count = 0;
    gaia::db::begin_transaction();
    for (auto student : gaia::prerequisites::student_t::list())
    {
        for (auto registration : student.registrations())
        {
            row_count++;
            auto course = registration.registered_course();
            // Enable the following statement when the issue of rule ordering is resolved. The corresponding
            // rule, on_insert(course) in test_insert_delete_2, should be enabled at the same time.
            // Currently when that rule and the on_insert(student) rule execute in different orders, the
            // results change and this test fails.
            // GAIAPLAT-1422
            // EXPECT_EQ(student.total_hours() * 2, course.hours());
            EXPECT_EQ(student.total_hours(), course.hours());
            EXPECT_STREQ(student.surname(), registration.status());
            EXPECT_EQ(registration.grade(), c_grade_d + c_grade_plus);
        }
    }
    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();
    EXPECT_EQ(row_count, 8) << "on_insert(enrollment_log) failed to create connections";
}
