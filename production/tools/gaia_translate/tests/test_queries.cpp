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

/**
 * Ensure that is possible to intermix cpp code with declarative code.
 */
class tools__gaia_translate__queries__test : public db_catalog_test_base_t
{
public:
    tools__gaia_translate__queries__test()
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
        g_insert_count = 0;
    }

    void TearDown() override
    {
        db_catalog_test_base_t::TearDown();
        unsubscribe_rules();
        gaia::rules::shutdown_rules_engine();
    }

    void mini_pop()
    {
        gaia::db::begin_transaction();

        student_t::get(student_t::insert_row("stu001", "Richard", 45, 4, 3.0));
        student_t::get(student_t::insert_row("stu002", "Russell", 32, 4, 3.0));
        student_t::get(student_t::insert_row("stu003", "Reuben", 26, 4, 3.0));

        course_t::get(course_t::insert_row("cou001", "math101", 3));
        course_t::get(course_t::insert_row("cou002", "math201", 4));
        course_t::get(course_t::insert_row("cou003", "eng101", 3));
        course_t::get(course_t::insert_row("cou004", "sci101", 3));
        course_t::get(course_t::insert_row("cou005", "math301", 5));

        registration_t::insert_row("reg001", "stu001", "cou002", c_status_pending, c_grade_none);
        registration_t::insert_row("reg002", "stu001", "cou004", c_status_eligible, c_grade_c);
        registration_t::insert_row("reg003", "stu002", "cou001", c_status_eligible, c_grade_b);
        registration_t::insert_row("reg004", "stu002", "cou003", c_status_eligible, c_grade_c);
        registration_t::insert_row("reg005", "stu002", "cou004", c_status_eligible, c_grade_d);
        registration_t::insert_row("reg006", "stu003", "cou005", c_status_pending, c_grade_none);
        registration_t::insert_row("reg007", "stu003", "cou002", c_status_eligible, c_grade_c);
        registration_t::insert_row("reg008", "stu003", "cou001", c_status_eligible, c_grade_b);
        gaia::db::commit_transaction();
    }

    void populate_db()
    {
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

        reg_1 = registration_t::get(registration_t::insert_row("reg001", "stu001", "cou002", c_status_pending, c_grade_none));
        // These are gaia_id_t.
        registration_t::insert_row("reg002", "stu001", "cou004", c_status_eligible, c_grade_c);
        registration_t::insert_row("reg003", "stu002", "cou001", c_status_eligible, c_grade_b);
        registration_t::insert_row("reg004", "stu002", "cou003", c_status_eligible, c_grade_c);
        registration_t::insert_row("reg005", "stu002", "cou004", c_status_eligible, c_grade_d);
        registration_t::insert_row("reg006", "stu003", "cou005", c_status_pending, c_grade_none);
        registration_t::insert_row("reg007", "stu003", "cou002", c_status_eligible, c_grade_c);
        registration_t::insert_row("reg008", "stu003", "cou001", c_status_eligible, c_grade_b);
        registration_t::insert_row("reg009", "stu003", "cou003", c_status_eligible, c_grade_b);
        registration_t::insert_row("reg00A", "stu003", "cou004", c_status_eligible, c_grade_a);
        registration_t::insert_row("reg00B", "stu004", "cou004", c_status_eligible, c_grade_b);
        registration_t::insert_row("reg00C", "stu004", "cou001", c_status_eligible, c_grade_b);
        registration_t::insert_row("reg00D", "stu005", "cou002", c_status_pending, c_grade_none);
        registration_t::insert_row("reg00E", "stu005", "cou003", c_status_eligible, c_grade_c);
        registration_t::insert_row("reg00F", "stu005", "cou004", c_status_eligible, c_grade_a);
        registration_t::insert_row("reg00G", "stu005", "cou001", c_status_eligible, c_grade_b);

        // These are gaia_id_t.
        auto prereq_1 = prereq_t::get(prereq_t::insert_row("pre001", c_grade_c));
        auto prereq_2 = prereq_t::get(prereq_t::insert_row("pre002", c_grade_d));
        auto prereq_3 = prereq_t::get(prereq_t::insert_row("pre003", c_grade_c));
        auto prereq_4 = prereq_t::get(prereq_t::insert_row("pre004", c_grade_c));

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

TEST_F(tools__gaia_translate__queries__test, basic_implicit_navigation)
{
    mini_pop();
    // populate_db();

    gaia::rules::subscribe_ruleset("test_queries");

    // Fire on_update(S:student).
    // student "Richard" has two classes with 3 and 4 hours.
    gaia::db::begin_transaction();
    auto s = student_t::list().where(surname == "Richard").begin();
    auto sw = s->writer();
    sw.age = 46;
    sw.update_row();
    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();
    EXPECT_TRUE(g_onupdate_called) << "on_update(S:student) not called";
    // Expected value is hours of math201 (4) plus sci101 (3).
    EXPECT_EQ(g_onupdate_value, 7) << "Incorrect sum";
}

TEST_F(tools__gaia_translate__queries__test, implicit_navigation_fork)
{
    populate_db();

    gaia::rules::subscribe_ruleset("test_queries");

    // Fire on_update(registration)
    // This registration is connected to student "Russell" (4 total_hours) and course "math101" (3 hours)
    gaia::db::begin_transaction();
    auto r = registration_t::list().where(reg_id == "reg003").begin();
    auto rw = r->writer();
    rw.grade = c_grade_c;
    rw.update_row();
    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();
    EXPECT_TRUE(g_onupdate_called) << "on_update(registration) not called";

    EXPECT_EQ(g_onupdate_value, 7) << "Incorrect sum";
}

TEST_F(tools__gaia_translate__queries__test, new_registration)
{
    // This is the number of successful registrations, based on prerequisite
    // minimum grade requirements.
    const int num_eligible = 1;

    populate_db();

    gaia::rules::subscribe_ruleset("test_queries");

    // The students will register for a class. The rule, on_insert(registration)
    // will decide the status of each registration.
    gaia::db::begin_transaction();

    // Richard registers for math301
    auto reg = registration_t::insert_row("reg00H", "stu001", "cou005", c_status_pending, c_grade_none);

    // Russell registers for math201.
    reg = registration_t::insert_row("reg00I", "stu002", "cou002", c_status_pending, c_grade_none);

    // Rachael register for eng101.
    reg = registration_t::insert_row("reg00K", "stu004", "cou003", c_status_pending, c_grade_none);

    // Renee registers for math301
    reg = registration_t::insert_row("reg00L", "stu005", "cou005", c_status_pending, c_grade_none);

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    EXPECT_EQ(g_insert_count, num_eligible)
        << "Expected " << num_eligible << " eligible registrations.";
    EXPECT_TRUE(g_oninsert_called) << "on_insert(registration) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert_result) << "on_insert failure";
}

TEST_F(tools__gaia_translate__queries__test, sum_of_ages)
{
    populate_db();

    gaia::rules::subscribe_ruleset("test_queries");

    // Fire on_insert(student). Expect to see a sum of all student ages.
    gaia::db::begin_transaction();

    auto student = student_t::get(student_t::insert_row("stu006", "Paul", 62, 4, 3.3));

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();
    EXPECT_TRUE(g_oninsert2_called) << "on_insert(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert2_result) << "on_insert failure";

    EXPECT_EQ(g_oninsert2_value, 281) << "Incorrect sum";
}

TEST_F(tools__gaia_translate__queries__test, sum_of_hours)
{
    populate_db();

    gaia::rules::subscribe_ruleset("test_query_1");

    // on_insert(registration) will sum up the student's hours.
    gaia::db::begin_transaction();

    registration_t::insert_row("reg00H", "stu001", "cou005", c_status_pending, c_grade_none);

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();
    EXPECT_TRUE(g_oninsert_called) << "on_insert(registration) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert_result) << "on_insert failure";

    EXPECT_EQ(g_oninsert_value, 5) << "Incorrect sum";
}

TEST_F(tools__gaia_translate__queries__test, sum_of_all_hours)
{
    populate_db();

    gaia::rules::subscribe_ruleset("test_query_2");

    // on_insert(registration) will sum up the student's hours.
    gaia::db::begin_transaction();

    registration_t::insert_row("reg00H", "stu001", "cou005", c_status_pending, c_grade_none);

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();
    EXPECT_TRUE(g_oninsert_called) << "on_insert(registration) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert_result) << "on_insert failure";

    EXPECT_EQ(g_oninsert_value, 12) << "Incorrect sum";
}

TEST_F(tools__gaia_translate__queries__test, tag_define_use)
{
    populate_db();

    gaia::rules::subscribe_ruleset("test_query_3");

    // on_insert(registration) will sum up the student's hours.
    gaia::db::begin_transaction();

    registration_t::insert_row("reg00H", "stu001", "cou005", c_status_pending, c_grade_none);

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();
    EXPECT_TRUE(g_oninsert_called) << "on_insert(registration) not called";
    EXPECT_TRUE(g_onupdate_called) << "on_update(course.hours) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert_result) << "on_insert failure";

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

TEST_F(tools__gaia_translate__queries__test, if_stmt)
{
    populate_db();

    gaia::rules::subscribe_ruleset("test_query_4");

    // on_insert(registration) will sum up the student's hours.
    gaia::db::begin_transaction();

    auto student = student_t::get(student_t::insert_row("stu006", "Paul", 62, 4, 3.3));

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();
    EXPECT_TRUE(g_oninsert_called) << "on_insert(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert_result) << "on_insert failure";

    EXPECT_EQ(g_oninsert_value, 0) << "Incorrect result";
}

TEST_F(tools__gaia_translate__queries__test, if_stmt2)
{
    const int num_updates = 5;

    populate_db();

    gaia::rules::subscribe_ruleset("test_query_4");

    // @hours - active variable.
    // Rule causes forward chanining, but terminates after 4 calls.
    g_onupdate_value = 0;
    gaia::db::begin_transaction();

    auto cw = course_1.writer();
    cw.hours = num_updates;
    cw.update_row();

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();
    EXPECT_EQ(g_onupdate_value, num_updates) << "on_update(course) not called '" << num_updates << "' times.";
    EXPECT_TRUE(g_onupdate_called) << "on_update(course) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate_result) << "on_update failure";

    EXPECT_EQ(g_onupdate_value, 5) << "Incorrect result";
}

TEST_F(tools__gaia_translate__queries__test, if_stmt3)
{
    populate_db();

    gaia::rules::subscribe_ruleset("test_query_4");

    // @grade - active variable.
    // Rule causes forward chanining, but terminates after 4 calls.
    g_string_value = "";
    gaia::db::begin_transaction();

    auto rw = reg_1.writer();
    rw.grade = c_grade_d;
    rw.update_row();

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();
    EXPECT_TRUE(g_onupdate_called) << "on_update(registration) not called";
    EXPECT_EQ(g_onupdate_value, 29) << "on_update(registration) incorrect result";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate_result) << "on_update failure";
}

TEST_F(tools__gaia_translate__queries__test, nomatch_stmt)
{
    populate_db();

    gaia::rules::subscribe_ruleset("test_query_6");

    // on_update(student) will fire a rule with an if/else/nomatch.
    g_string_value = "";
    gaia::db::begin_transaction();

    auto sw = student_1.writer();
    sw.gpa = 3.5;
    sw.update_row();

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();
    EXPECT_TRUE(g_onupdate_called) << "on_update(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate_result) << "on_update failure";

    EXPECT_EQ(g_string_value, "found Richard") << "Incorrect result";
}

TEST_F(tools__gaia_translate__queries__test, nomatch_stmt2)
{
    populate_db();

    gaia::rules::subscribe_ruleset("test_query_6");

    // on_insert(student) will look for class hours, which don't exist - nomatch!
    g_string_value = "";
    gaia::db::begin_transaction();

    auto student = student_t::get(student_t::insert_row("stu006", "Paul", 62, 4, 3.3));

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();
    EXPECT_TRUE(g_onupdate_called) << "on_update(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate_result) << "on_update failure";

    EXPECT_EQ(g_string_value, "nomatch success") << "Incorrect result";
}

TEST_F(tools__gaia_translate__queries__test, nomatch_stmt3)
{
    populate_db();

    gaia::rules::subscribe_ruleset("test_query_7");

    // on_insert(registration) will look for class hours, which don't exist - nomatch!
    g_string_value = "";
    gaia::db::begin_transaction();

    auto student = student_t::get(student_t::insert_row("stu006", "Paul", 62, 4, 3.3));
    registration_t::insert_row("reg00H", "stu006", nullptr, c_status_eligible, c_grade_c);

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();
    EXPECT_TRUE(g_oninsert_called) << "on_update(registration) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert_result) << "on_update failure";

    EXPECT_EQ(g_string_value, "correct nomatch") << "Incorrect result";
}

TEST_F(tools__gaia_translate__queries__test, nomatch_stmt4)
{
    populate_db();

    gaia::rules::subscribe_ruleset("test_query_8");

    // on_insert(student) will look for class hours, which don't exist - nomatch!
    g_string_value = "";
    gaia::db::begin_transaction();

    auto student = student_t::get(student_t::insert_row("stu006", "Paul", 62, 4, 3.3));

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();
    EXPECT_TRUE(g_oninsert_called) << "on_update(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert_result) << "on_update failure";

    EXPECT_EQ(g_string_value, "nomatch success") << "Incorrect result";
}

TEST_F(tools__gaia_translate__queries__test, nomatch_function_query)
{
    populate_db();

    gaia::rules::subscribe_ruleset("test_query_9");

    g_string_value = "";
    gaia::db::begin_transaction();

    auto student = student_t::get(student_t::insert_row("stu006", "Paul", 62, 4, 3.3));
    auto registration = registration_t::insert_row("reg00H", "stu006", "cou001", c_status_eligible, c_grade_c);
    registration = registration_t::insert_row("reg00I", "stu006", "cou002", c_status_eligible, c_grade_d);

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();
    EXPECT_TRUE(g_oninsert_called) << "on_insert(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert_result) << "on_insert failure";

    EXPECT_EQ(g_string_value, "4 2.0 3 ") << "Incorrect result";
}

TEST_F(tools__gaia_translate__queries__test, one_to_one)
{
    gaia::db::begin_transaction();
    student_1 = student_t::get(student_t::insert_row("stu001", "Richard", 45, 4, 3.0));
    student_2 = student_t::get(student_t::insert_row("stu002", "Russell", 32, 4, 3.0));
    student_3 = student_t::get(student_t::insert_row("stu003", "Reuben", 26, 4, 3.0));
    student_4 = student_t::get(student_t::insert_row("stu004", "Rachael", 51, 4, 3.0));
    student_5 = student_t::get(student_t::insert_row("stu005", "Renee", 65, 4, 3.0));

    // Create and connect 1:1 parents for 3 of the students.
    auto parents_1 = parents_t::get(parents_t::insert_row("Lawrence", "Elizabeth"));
    auto parents_2 = parents_t::get(parents_t::insert_row("Clarence", "Pauline"));
    auto parents_3 = parents_t::get(parents_t::insert_row("George", "Irvie"));

    student_1.parents().connect(parents_1);
    student_3.parents().connect(parents_2);
    student_5.parents().connect(parents_3);
    gaia::db::commit_transaction();

    gaia::rules::subscribe_ruleset("test_query_10");

    g_string_value = "";
    gaia::db::begin_transaction();
    auto student = student_t::get(student_t::insert_row("stu006", "Paul", 62, 4, 3.3));
    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();
    EXPECT_TRUE(g_oninsert_called) << "on_update(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert_result) << "on_update failure";

    EXPECT_EQ(g_oninsert_value, 6) << "Incorrect result";
}

// Query tests:
//  - single-statement loop over records owned by anchor.
//  - for loop over records owned by anchor.
//  - if loop over records owned by anchor.
//  - loop over records owned by records owned by anchor.

//  - implicit query in function call.
//  - explicit query in function call.
