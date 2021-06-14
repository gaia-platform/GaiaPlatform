/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <unistd.h>

#include <atomic>

#include "gtest/gtest.h"

#include "gaia/rules/rules.hpp"

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_prerequisites.h"
#include "test_rulesets.hpp"

using namespace std;
using namespace gaia::prerequisites;
using namespace gaia::prerequisites::student_expr;
using namespace gaia::prerequisites::registration_expr;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::rules;
using namespace rule_test_helpers;

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

/**
 * Ensure that is possible to intermix cpp code with declarative code.
 */
class test_queries_code : public db_catalog_test_base_t
{
public:
    test_queries_code()
        : db_catalog_test_base_t("prerequisites.ddl"){};

protected:
    void SetUp() override
    {
        db_catalog_test_base_t::SetUp();
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
        gaia::db::begin_transaction();

        // These must be EDC objects. They have insert() methods.
        student_1 = student_t::get(student_t::insert_row("stu001", "Richard", 45, 4, 3.0));
        student_2 = student_t::get(student_t::insert_row("stu002", "Russell", 32, 4, 3.0));
        student_3 = student_t::get(student_t::insert_row("stu003", "Reuben", 26, 4, 3.0));
        student_4 = student_t::get(student_t::insert_row("stu004", "Rachael", 51, 4, 3.0));
        student_5 = student_t::get(student_t::insert_row("stu005", "Renee", 65, 4, 3.0));

        // These must be EDC objects. They have insert() methods.
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
    }
};

TEST_F(test_queries_code, basic_implicit_navigation)
{
    populate_db();

    gaia::rules::initialize_rules_engine();
    // Use the second set of rules.
    gaia::rules::unsubscribe_rules();
    gaia::rules::subscribe_ruleset("test_queries");

    // Fire OnUpdate(S:student).
    // student "Richard" has two classes with 3 and 4 hours.
    gaia::db::begin_transaction();
    auto s = student_t::list().where(surname == "Richard").begin();
    auto sw = s->writer();
    sw.age = 46;
    sw.update_row();
    gaia::db::commit_transaction();

    EXPECT_TRUE(wait_for_rule(g_onupdate_called)) << "OnUpdate(S:student) not called";
    // Expected value is hours of math201 (4) plus sci101 (3).
    EXPECT_EQ(g_onupdate_value, 7) << "Incorrect sum";
}

TEST_F(test_queries_code, implicit_navigation_fork)
{
    populate_db();

    gaia::rules::initialize_rules_engine();
    // Use the second set of rules.
    gaia::rules::unsubscribe_rules();
    gaia::rules::subscribe_ruleset("test_queries");

    // Fire OnUpdate(registration)
    // This registration is connected to student "Russell" (4 total_hours) and course "math101" (3 hours)
    gaia::db::begin_transaction();
    auto r = registration_t::list().where(reg_id == "reg003").begin();
    auto rw = r->writer();
    rw.grade = c_grade_c;
    rw.update_row();
    gaia::db::commit_transaction();

    EXPECT_TRUE(wait_for_rule(g_onupdate_called)) << "OnUpdate(registration) not called";
    EXPECT_EQ(g_onupdate_value, 7) << "Incorrect sum";
}

TEST_F(test_queries_code, new_registration)
{
    const int num_inserts = 4;

    populate_db();

    gaia::rules::initialize_rules_engine();
    // Use the second set of rules.
    gaia::rules::unsubscribe_rules();
    gaia::rules::subscribe_ruleset("test_queries");

    // The students will register for a class. The rule, OnInsert(registration)
    // will decide the status of each registration.
    gaia::db::begin_transaction();

    // Richard registers for math301
    auto reg = registration_t::insert_row("reg00H", c_status_pending, c_grade_none);
    student_1.registrations().insert(reg);
    course_5.registrations().insert(reg);

    // Russell registers for math201.
    reg = registration_t::insert_row("reg00I", c_status_pending, c_grade_none);
    student_2.registrations().insert(reg);
    course_2.registrations().insert(reg);

    // Rachael register for eng101.
    reg = registration_t::insert_row("reg00K", c_status_pending, c_grade_none);
    // auto student_4 = student_t::get(student_t::insert_row("stu004", "Rachael", 51, 4, 3.0));
    student_4.registrations().insert(reg);
    course_3.registrations().insert(reg);

    // Renee registers for math301
    reg = registration_t::insert_row("reg00L", c_status_pending, c_grade_none);
    // auto student_5 = student_t::get(student_t::insert_row("stu005", "Renee", 65, 4, 3.0));
    student_5.registrations().insert(reg);
    course_5.registrations().insert(reg);

    gaia::db::commit_transaction();

    EXPECT_TRUE(wait_for_rule(g_insert_count, num_inserts))
        << "OnInsert(registration) not called '" << num_inserts << "'times.";
    EXPECT_TRUE(wait_for_rule(g_oninsert_called)) << "OnInsert(registration) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert_result) << "OnInsert failure";
}

TEST_F(test_queries_code, sum_of_ages)
{
    populate_db();

    gaia::rules::initialize_rules_engine();
    // Use the second set of rules.
    gaia::rules::unsubscribe_rules();
    gaia::rules::subscribe_ruleset("test_queries");

    // Fire OnInsert(student). Expect to see a sum of all student ages.
    gaia::db::begin_transaction();

    auto student = student_t::get(student_t::insert_row("stu006", "Paul", 62, 4, 3.3));

    gaia::db::commit_transaction();

    EXPECT_TRUE(wait_for_rule(g_oninsert2_called)) << "OnInsert(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert2_result) << "OnInsert failure";

    EXPECT_EQ(g_oninsert2_value, 281) << "Incorrect sum";
}

TEST_F(test_queries_code, sum_of_hours)
{
    populate_db();

    gaia::rules::initialize_rules_engine();
    // Use the second set of rules.
    gaia::rules::unsubscribe_rules();
    gaia::rules::subscribe_ruleset("test_query_1");

    // OnInsert(registration) will sum up the student's hours.
    gaia::db::begin_transaction();

    auto reg = registration_t::insert_row("reg00H", c_status_pending, c_grade_none);
    student_1.registrations().insert(reg);
    course_5.registrations().insert(reg);

    gaia::db::commit_transaction();

    EXPECT_TRUE(wait_for_rule(g_oninsert_called)) << "OnInsert(registration) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert_result) << "OnInsert failure";

    EXPECT_EQ(g_oninsert_value, 5) << "Incorrect sum";
}

TEST_F(test_queries_code, sum_of_all_hours)
{
    populate_db();

    gaia::rules::initialize_rules_engine();
    // Use the second set of rules.
    gaia::rules::unsubscribe_rules();
    gaia::rules::subscribe_ruleset("test_query_2");

    // OnInsert(registration) will sum up the student's hours.
    gaia::db::begin_transaction();

    auto reg = registration_t::insert_row("reg00H", c_status_pending, c_grade_none);
    student_1.registrations().insert(reg);
    course_5.registrations().insert(reg);

    gaia::db::commit_transaction();

    EXPECT_TRUE(wait_for_rule(g_oninsert_called)) << "OnInsert(registration) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert_result) << "OnInsert failure";

    EXPECT_EQ(g_oninsert_value, 12) << "Incorrect sum";
}

TEST_F(test_queries_code, tag_define_use)
{
    populate_db();

    gaia::rules::initialize_rules_engine();
    // Use the second set of rules.
    gaia::rules::unsubscribe_rules();
    gaia::rules::subscribe_ruleset("test_query_3");

    // OnInsert(registration) will sum up the student's hours.
    gaia::db::begin_transaction();

    auto reg = registration_t::insert_row("reg00H", c_status_pending, c_grade_none);
    student_1.registrations().insert(reg);
    course_5.registrations().insert(reg);

    gaia::db::commit_transaction();

    EXPECT_TRUE(wait_for_rule(g_oninsert_called)) << "OnInsert(registration) not called";
    EXPECT_TRUE(wait_for_rule(g_onupdate_called)) << "OnUpdate(course.hours) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert_result) << "OnInsert failure";

    // Examinine values. We know the answers already.
    int loop_num = 0;
    gaia::db::begin_transaction();
    for (auto& r : student_1.registrations())
    {
        auto c = r.registered_course();
        switch (loop_num)
        {
        case 0:
            EXPECT_EQ(c.hours(), 9);
            break;
        case 1:
            EXPECT_EQ(c.hours(), 7);
            break;
        case 2:
            EXPECT_EQ(c.hours(), 8);
            break;
        }
        loop_num++;
    }
    gaia::db::commit_transaction();
}

TEST_F(test_queries_code, if_stmt)
{
    populate_db();

    gaia::rules::initialize_rules_engine();
    // Use the second set of rules.
    gaia::rules::unsubscribe_rules();
    gaia::rules::subscribe_ruleset("test_query_4");

    // OnInsert(registration) will sum up the student's hours.
    gaia::db::begin_transaction();

    auto student = student_t::get(student_t::insert_row("stu006", "Paul", 62, 4, 3.3));

    gaia::db::commit_transaction();

    EXPECT_TRUE(wait_for_rule(g_oninsert_called)) << "OnInsert(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert_result) << "OnInsert failure";

    EXPECT_EQ(g_oninsert_value, 0) << "Incorrect result";
}

TEST_F(test_queries_code, if_stmt2)
{
    const int num_updates = 5;

    populate_db();

    gaia::rules::initialize_rules_engine();
    // Use the second set of rules.
    gaia::rules::unsubscribe_rules();
    gaia::rules::subscribe_ruleset("test_query_4");

    // @hours - active variable.
    // Rule causes forward chanining, but terminates after 4 calls.
    g_onupdate_value = 0;
    gaia::db::begin_transaction();

    auto cw = course_1.writer();
    cw.hours = num_updates;
    cw.update_row();

    gaia::db::commit_transaction();

    EXPECT_TRUE(wait_for_rule(g_onupdate_value, num_updates)) << "OnUpdate(course) not called '" << num_updates << "' times.";
    EXPECT_TRUE(wait_for_rule(g_onupdate_called)) << "OnUpdate(course) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate_result) << "OnUpdate failure";

    EXPECT_EQ(g_onupdate_value, 5) << "Incorrect result";
}

TEST_F(test_queries_code, if_stmt3)
{
    populate_db();

    gaia::rules::initialize_rules_engine();
    // Use the second set of rules.
    gaia::rules::unsubscribe_rules();
    gaia::rules::subscribe_ruleset("test_query_4");

    // @grade - active variable.
    // Rule causes forward chanining, but terminates after 4 calls.
    g_string_value = "";
    gaia::db::begin_transaction();

    auto rw = reg_1.writer();
    rw.grade = "D";
    rw.update_row();

    gaia::db::commit_transaction();

    EXPECT_TRUE(wait_for_rule(g_onupdate_called)) << "OnUpdate(registration) not called";
    EXPECT_EQ(g_onupdate_value, 29) << "OnUpdate(registration) incorrect result";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate_result) << "OnUpdate failure";
}

TEST_F(test_queries_code, while_stmt)
{
    populate_db();

    gaia::rules::initialize_rules_engine();
    // Use the second set of rules.
    gaia::rules::unsubscribe_rules();
    gaia::rules::subscribe_ruleset("test_query_5");

    // @hours - active variable.
    // Rule causes forward chanining, but terminates after 4 calls.
    g_string_value = "";
    gaia::db::begin_transaction();

    auto sw = student_1.writer();
    sw.gpa = 3.5;
    sw.update_row();

    gaia::db::commit_transaction();

    EXPECT_TRUE(wait_for_rule(g_onupdate_called)) << "OnUpdate(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate_result) << "OnUpdate failure";

    EXPECT_EQ(g_string_value, "stu001reg002cou004 stu001reg001cou002 ") << "Incorrect result";
}

TEST_F(test_queries_code, nomatch_stmt)
{
    populate_db();

    gaia::rules::initialize_rules_engine();
    // Use the second set of rules.
    gaia::rules::unsubscribe_rules();
    gaia::rules::subscribe_ruleset("test_query_6");

    // OnUpdate(student) will fire a rule with an if/else/nomatch.
    g_string_value = "";
    gaia::db::begin_transaction();

    auto sw = student_1.writer();
    sw.gpa = 3.5;
    sw.update_row();

    gaia::db::commit_transaction();

    EXPECT_TRUE(wait_for_rule(g_onupdate_called)) << "OnUpdate(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate_result) << "OnUpdate failure";

    EXPECT_EQ(g_string_value, "found Richard") << "Incorrect result";
}

TEST_F(test_queries_code, nomatch_stmt2)
{
    populate_db();

    gaia::rules::initialize_rules_engine();
    // Use the second set of rules.
    gaia::rules::unsubscribe_rules();
    gaia::rules::subscribe_ruleset("test_query_6");

    // OnInsert(student) will look for class hours, which don't exist - nomatch!
    g_string_value = "";
    gaia::db::begin_transaction();

    auto student = student_t::get(student_t::insert_row("stu006", "Paul", 62, 4, 3.3));

    gaia::db::commit_transaction();

    EXPECT_TRUE(wait_for_rule(g_onupdate_called)) << "OnUpdate(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate_result) << "OnUpdate failure";

    EXPECT_EQ(g_string_value, "nomatch success") << "Incorrect result";
}

// Re-enable when GAIAPLAT-1005 is fixed.
TEST_F(test_queries_code, nomatch_stmt3)
{
    populate_db();

    gaia::rules::initialize_rules_engine();
    // Use the second set of rules.
    gaia::rules::unsubscribe_rules();
    gaia::rules::subscribe_ruleset("test_query_7");

    // OnInsert(registration) will look for class hours, which don't exist - nomatch!
    g_string_value = "";
    gaia::db::begin_transaction();

    auto student = student_t::get(student_t::insert_row("stu006", "Paul", 62, 4, 3.3));
    auto registration = registration_t::insert_row("reg00H", c_status_eligible, c_grade_c);
    student.registrations().insert(registration);

    gaia::db::commit_transaction();

    EXPECT_TRUE(wait_for_rule(g_oninsert_called)) << "OnUpdate(registration) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert_result) << "OnUpdate failure";

    EXPECT_EQ(g_string_value, "correct nomatch") << "Incorrect result";
}

TEST_F(test_queries_code, nomatch_stmt4)
{
    populate_db();

    gaia::rules::initialize_rules_engine();
    // Use the second set of rules.
    gaia::rules::unsubscribe_rules();
    gaia::rules::subscribe_ruleset("test_query_8");

    // OnInsert(student) will look for class hours, which don't exist - nomatch!
    g_string_value = "";
    gaia::db::begin_transaction();

    auto student = student_t::get(student_t::insert_row("stu006", "Paul", 62, 4, 3.3));

    gaia::db::commit_transaction();

    EXPECT_TRUE(wait_for_rule(g_oninsert_called)) << "OnUpdate(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert_result) << "OnUpdate failure";

    EXPECT_EQ(g_string_value, "nomatch success") << "Incorrect result";
}

TEST_F(test_queries_code, nomatch_function_query)
{
    populate_db();

    gaia::rules::initialize_rules_engine();
    // Use the second set of rules.
    gaia::rules::unsubscribe_rules();
    gaia::rules::subscribe_ruleset("test_query_9");

    // OnInsert(student) will look for class hours, which don't exist - nomatch!
    g_string_value = "";
    gaia::db::begin_transaction();

    auto student = student_t::get(student_t::insert_row("stu006", "Paul", 62, 4, 3.3));
    auto registration = registration_t::insert_row("reg00H", c_status_eligible, c_grade_c);
    student.registrations().insert(registration);
    course_1.registrations().insert(registration);
    registration = registration_t::insert_row("reg00I", c_status_eligible, c_grade_d);
    student.registrations().insert(registration);
    course_2.registrations().insert(registration);

    gaia::db::commit_transaction();

    EXPECT_TRUE(wait_for_rule(g_oninsert_called)) << "OnInsert(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert_result) << "OnInsert failure";

    EXPECT_EQ(g_string_value, "4C3 ") << "Incorrect result";
}

// Query tests:
//  - single-statement loop over records owned by anchor.
//  - for loop over records owned by anchor.
//  - if loop over records owned by anchor.
//  - loop over records owned by records owned by anchor.

//  - implicit query in function call.
//  - explicit query in function call.
