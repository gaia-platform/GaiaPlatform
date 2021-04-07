/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <unistd.h>

#include "gtest/gtest.h"

#include "gaia/rules/rules.hpp"

#include "gaia_internal/db/db_catalog_test_base.hpp"

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
extern int32_t g_onupdate_value;
extern int32_t g_onupdate3_value;

const int c_rule_execution_step_delay = 5000;
const int c_rule_execution_total_delay = 25000;

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
    }

    void TearDown() override
    {
        db_catalog_test_base_t::TearDown();
        unsubscribe_rules();
        gaia::rules::shutdown_rules_engine();
    }

    // Synchronize with the rules threads. Timeout quickly.
    bool wait_for_rule(bool& rule_called)
    {
        int rule_delay;

        for (rule_delay = 0;
             !rule_called && rule_delay <= c_rule_execution_total_delay;
             rule_delay += c_rule_execution_step_delay)
        {
            usleep(c_rule_execution_step_delay);
        }

        return rule_delay <= c_rule_execution_total_delay;
    }
};

TEST_F(test_tags_code, oninsert)
{
    gaia::rules::initialize_rules_engine();
    // Use the first set of rules.
    gaia::rules::unsubscribe_ruleset("test_queries");

    // Creating a record should fire OnInsert and OnChange, but not OnUpdate.
    gaia::db::begin_transaction();
    auto Student = Student_t::get(Student_t::insert_row("stu001", "Warren", 66, 3, 2.9));
    gaia::db::commit_transaction();

    // Check OnInsert.
    EXPECT_TRUE(wait_for_rule(g_oninsert_called)) << "OnInsert(Student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert_result) << "OnInsert failure";

    // Check second OnInsert.
    EXPECT_TRUE(wait_for_rule(g_oninsert2_called)) << "Second OnInsert(Student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert2_result) << "OnInsert failure";

    // Check OnChange.
    EXPECT_TRUE(wait_for_rule(g_onchange_called)) << "OnChange(Student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onchange_result) << "OnChange failure";

    EXPECT_TRUE(wait_for_rule(g_onchange2_called)) << "OnChange(Student.Age) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onchange2_result) << "OnChange failure";

    // Check OnUpdate.
    EXPECT_FALSE(wait_for_rule(g_onupdate_called)) << "OnUpdate(Student) should not be called";
    EXPECT_FALSE(wait_for_rule(g_onupdate2_called)) << "OnUpdate(Student.Surname) should not be called";
    EXPECT_FALSE(wait_for_rule(g_onupdate3_called)) << "OnUpdate(Student.Age) should not be called";
    EXPECT_FALSE(wait_for_rule(g_onupdate4_called)) << "OnUpdate(Student.Age, Student.Surname) should not be called";
}

TEST_F(test_tags_code, onchange)
{
    gaia::db::begin_transaction();
    auto Student = Student_t::get(Student_t::insert_row("stu001", "Warren", 66, 3, 2.9));
    gaia::db::commit_transaction();

    gaia::rules::initialize_rules_engine();
    // Use the first set of rules.
    gaia::rules::unsubscribe_ruleset("test_queries");

    // Changing a record should fire OnChange and OnUpdate, but not OnInsert.
    gaia::db::begin_transaction();
    auto sw = Student.writer();
    sw.Surname = "Hawkins";
    sw.update_row();
    gaia::db::commit_transaction();

    // Check OnInsert.
    EXPECT_FALSE(wait_for_rule(g_oninsert_called)) << "OnInsert(Student) called after field write";
    EXPECT_FALSE(wait_for_rule(g_oninsert2_called)) << "Second OnInsert(Student) called after field write";

    // Check OnChange.
    EXPECT_TRUE(wait_for_rule(g_onchange_called)) << "OnChange(Student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onchange_result) << "OnChange failure";

    EXPECT_FALSE(wait_for_rule(g_onchange2_called)) << "OnChange(Student.Age) should not be called";

    // Check OnUpdate of Student.
    EXPECT_TRUE(wait_for_rule(g_onupdate_called)) << "OnUpdate(Student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate_result) << "OnUpdate failure";

    // Check OnUpdate of Student.Surname.
    EXPECT_TRUE(wait_for_rule(g_onupdate2_called)) << "OnUpdate(Student.Surname) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate2_result) << "OnUpdate failure";

    // Check OnUpdate of Student.Age.
    EXPECT_FALSE(wait_for_rule(g_onupdate3_called)) << "OnUpdate(Student.Age) should not be called";

    // Check OnUpdate of Student.Age or Student.Surname.
    EXPECT_TRUE(wait_for_rule(g_onupdate4_called)) << "OnUpdate(Student.Age, Student.Surname) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate4_result) << "OnUpdate failure";
}

TEST_F(test_tags_code, onupdate)
{
    gaia::db::begin_transaction();
    auto Student = Student_t::get(Student_t::insert_row("stu001", "Warren", 66, 3, 2.9));
    gaia::db::commit_transaction();

    gaia::rules::initialize_rules_engine();
    // Use the first set of rules.
    gaia::rules::unsubscribe_ruleset("test_queries");

    // Changing the Age field should fire OnChange and OnUpdate, but not OnInsert.
    gaia::db::begin_transaction();
    auto sw = Student.writer();
    sw.Age = 45;
    sw.update_row();
    gaia::db::commit_transaction();

    // Check OnInsert.
    EXPECT_FALSE(wait_for_rule(g_oninsert_called)) << "OnInsert(Student) called after field write";
    EXPECT_FALSE(wait_for_rule(g_oninsert2_called)) << "Second OnInsert(Student) called after field write";

    // Check OnChange.
    EXPECT_TRUE(wait_for_rule(g_onchange_called)) << "OnChange(Student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onchange_result) << "OnChange failure";

    EXPECT_TRUE(wait_for_rule(g_onchange2_called)) << "OnChange(Student.Age) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onchange2_result) << "OnChange failure";

    // Check OnUpdate of Student.
    EXPECT_TRUE(wait_for_rule(g_onupdate_called)) << "OnUpdate(Student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate_result) << "OnUpdate failure";

    // Check OnUpdate of Student.Surname.
    EXPECT_TRUE(wait_for_rule(g_onupdate3_called)) << "OnUpdate(Student.Age) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate3_result) << "OnUpdate failure";

    // Check OnUpdate of Student.Age.
    EXPECT_FALSE(wait_for_rule(g_onupdate2_called)) << "OnUpdate(Student.Surname) not called";
}

TEST_F(test_tags_code, basic_tags)
{
    gaia::rules::initialize_rules_engine();
    // Use the first set of rules.
    gaia::rules::unsubscribe_ruleset("test_queries");

    gaia::db::begin_transaction();
    Registration_t::insert_row("reg00H", "pending", "");
    gaia::db::commit_transaction();

    EXPECT_TRUE(wait_for_rule(g_oninsert3_called)) << "OnInsert(Registration) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_oninsert3_result) << "OnInsert(Registration) failure";
}
