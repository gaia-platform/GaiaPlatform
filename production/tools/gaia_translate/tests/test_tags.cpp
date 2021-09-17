/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"

#include "gaia/rules/rules.hpp"

#include "gaia_internal/db/db_catalog_test_base.hpp"
#include "gaia_internal/rules/rules_test_helpers.hpp"

#include "gaia_prerequisites.h"
#include "test_rulesets.hpp"

using namespace std;
using namespace gaia::prerequisites;
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

extern std::atomic<int32_t> g_insert_count;
extern std::atomic<int32_t> g_onupdate_value;
/**
 * Ensure that is possible to intermix cpp code with declarative code.
 */
class test_tags_code : public db_catalog_test_base_t
{
public:
    test_tags_code()
        : db_catalog_test_base_t("prerequisites.ddl"){};

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
};

TEST_F(test_tags_code, oninsert)
{
    gaia::rules::subscribe_ruleset("test_tags");

    // Creating a record should fire on_insert and on_change, but not on_update.
    gaia::db::begin_transaction();
    auto student = student_t::get(student_t::insert("stu001", "Warren", 66, 3, 2.9));
    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    // Check on_insert.
    EXPECT_TRUE(g_oninsert_called) << "on_insert(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert_result) << "on_insert failure";

    // Check second on_insert.
    EXPECT_TRUE(g_oninsert2_called) << "Second on_insert(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert2_result) << "on_insert failure";

    // Check on_change.
    EXPECT_TRUE(g_onchange_called) << "on_change(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onchange_result) << "on_change failure";

    EXPECT_TRUE(g_onchange2_called) << "on_change(student.age) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onchange2_result) << "on_change failure";

    // Check on_update.
    EXPECT_FALSE(g_onupdate_called) << "on_update(student) should not be called";
    EXPECT_FALSE(g_onupdate2_called) << "on_update(student.surname) should not be called";
    EXPECT_FALSE(g_onupdate3_called) << "on_update(student.age) should not be called";
    EXPECT_FALSE(g_onupdate4_called) << "on_update(student.age, student.surname) should not be called";
}

TEST_F(test_tags_code, onchange)
{
    gaia::db::begin_transaction();
    auto student = student_t::get(student_t::insert("stu001", "Warren", 66, 3, 2.9));
    gaia::db::commit_transaction();

    // Use the first set of rules.
    gaia::rules::subscribe_ruleset("test_tags");

    // Changing a record should fire on_change and on_update, but not on_insert.
    gaia::db::begin_transaction();
    auto sw = student.writer();
    sw.surname = "Hawkins";
    sw.update();
    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    // Check on_change.
    EXPECT_TRUE(g_onchange_called) << "on_change(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onchange_result) << "on_change failure";

    // Check on_update of student.
    EXPECT_TRUE(g_onupdate_called) << "on_update(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate_result) << "on_update failure";

    // Check on_update of student.surname.
    EXPECT_TRUE(g_onupdate2_called) << "on_update(student.surname) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate2_result) << "on_update failure";

    // Check on_update of student.age or student.surname.
    EXPECT_TRUE(g_onupdate4_called) << "on_update(student.age, student.surname) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate4_result) << "on_update failure";

    // Verify that these rules should *not* be called.

    // Check on_insert.
    EXPECT_FALSE(g_oninsert_called) << "on_insert(student) called after field write";
    EXPECT_FALSE(g_oninsert2_called) << "Second on_insert(student) called after field write";
    EXPECT_FALSE(g_onchange2_called) << "on_change(student.age) should not be called";
    EXPECT_FALSE(g_onupdate3_called) << "on_update(student.age) should not be called";
}

TEST_F(test_tags_code, onupdate)
{
    gaia::db::begin_transaction();
    auto student = student_t::get(student_t::insert("stu001", "Warren", 66, 3, 2.9));
    gaia::db::commit_transaction();

    // Use the first set of rules.
    gaia::rules::subscribe_ruleset("test_tags");

    // Changing the age field should fire on_change and on_update, but not on_insert.
    gaia::db::begin_transaction();
    auto sw = student.writer();
    sw.age = 45;
    sw.update();
    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    // Check on_change.
    EXPECT_TRUE(g_onchange_called) << "on_change(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onchange_result) << "on_change failure";

    EXPECT_TRUE(g_onchange2_called) << "on_change(student.age) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onchange2_result) << "on_change failure";

    // Check on_update of student.
    EXPECT_TRUE(g_onupdate_called) << "on_update(student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate_result) << "on_update failure";

    // Check on_update of student.age.
    EXPECT_TRUE(g_onupdate3_called) << "on_update(student.age) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate3_result) << "on_update failure";

    // Check on_update of student.age or student.surname.
    EXPECT_TRUE(g_onupdate4_called) << "on_update(student.age, student.surname) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate4_result) << "on_update failure";

    // Check on_update of student.surname.
    EXPECT_FALSE(g_onupdate2_called) << "on_update(student.surname) not called";

    // Check on_insert.
    EXPECT_FALSE(g_oninsert_called) << "on_insert(student) called after field write";
    EXPECT_FALSE(g_oninsert2_called) << "Second on_insert(student) called after field write";
}

TEST_F(test_tags_code, multi_inserts)
{
    const int num_inserts = 5;

    // Use the first set of rules.
    gaia::rules::unsubscribe_rules();
    gaia::rules::subscribe_ruleset("test_tags");

    gaia::db::begin_transaction();
    student_t::get(student_t::insert("stu001", "Richard", 45, 4, 3.0));
    student_t::get(student_t::insert("stu002", "Russell", 32, 4, 3.0));
    student_t::get(student_t::insert("stu003", "Reuben", 26, 4, 3.0));
    student_t::get(student_t::insert("stu004", "Rachael", 51, 4, 3.0));
    student_t::get(student_t::insert("stu005", "Renee", 65, 4, 3.0));
    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    EXPECT_EQ(g_insert_count, num_inserts) << "on_insert(student) parallel execution failure";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert_result) << "on_insert(student) failure";
}

TEST_F(test_tags_code, basic_tags)
{
    // Use the first set of rules.
    gaia::rules::subscribe_ruleset("test_tags");

    gaia::db::begin_transaction();
    registration_t::insert("reg00H", nullptr, nullptr, c_status_pending, c_grade_none);
    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    EXPECT_TRUE(g_oninsert3_called) << "on_insert(registration) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert3_result) << "on_insert(registration) failure";
}
