/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "test_rulesets.hpp"

#include <unistd.h>

#include "gtest/gtest.h"

#include "gaia/rules/rules.hpp"

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_prerequisites.h"

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

    void populate_db()
    {
        // These must be EDC objects. They have insert() methods.
        auto student_1 = Student_t::get(Student_t::insert_row("stu001", "Richard", 45, 4, 3.0));
        auto student_2 = Student_t::get(Student_t::insert_row("stu002", "Russell", 32, 4, 3.0));
        auto student_3 = Student_t::get(Student_t::insert_row("stu003", "Reuben", 26, 4, 3.0));
        auto student_4 = Student_t::get(Student_t::insert_row("stu004", "Rachael", 51, 4, 3.0));
        auto student_5 = Student_t::get(Student_t::insert_row("stu005", "Renee", 65, 4, 3.0));

        // These must be EDC objects. They have insert() methods.
        auto course_1 = Course_t::get(Course_t::insert_row("cou001", "math101", 3));
        auto course_2 = Course_t::get(Course_t::insert_row("cou002", "math201", 4));
        auto course_3 = Course_t::get(Course_t::insert_row("cou003", "eng101", 3));
        auto course_4 = Course_t::get(Course_t::insert_row("cou004", "sci101", 3));
        auto course_5 = Course_t::get(Course_t::insert_row("cou005", "math301", 5));

        // These are gaia_id_t.
        auto reg_1 = Registration_t::insert_row("reg001", "pending", "");
        auto reg_2 = Registration_t::insert_row("reg002", "eligible", "C");
        auto reg_3 = Registration_t::insert_row("reg003", "eligible", "B");
        auto reg_4 = Registration_t::insert_row("reg004", "eligible", "C");
        auto reg_5 = Registration_t::insert_row("reg005", "eligible", "D");
        auto reg_6 = Registration_t::insert_row("reg006", "pending", "");
        auto reg_7 = Registration_t::insert_row("reg007", "eligible", "C");
        auto reg_8 = Registration_t::insert_row("reg008", "eligible", "B");
        auto reg_9 = Registration_t::insert_row("reg009", "eligible", "B");
        auto reg_A = Registration_t::insert_row("reg00A", "eligible", "A");
        auto reg_B = Registration_t::insert_row("reg00B", "eligible", "B");
        auto reg_C = Registration_t::insert_row("reg00C", "eligible", "B");
        auto reg_D = Registration_t::insert_row("reg00D", "pending", "");
        auto reg_E = Registration_t::insert_row("reg00E", "eligible", "C");
        auto reg_F = Registration_t::insert_row("reg00F", "eligible", "A");
        auto reg_G = Registration_t::insert_row("reg00G", "eligible", "B");

        // These are gaia_id_t.
        auto prereq_1 = PreReq_t::insert_row("pre001", "C");
        auto prereq_2 = PreReq_t::insert_row("pre002", "D");
        auto prereq_3 = PreReq_t::insert_row("pre003", "C");
        auto prereq_4 = PreReq_t::insert_row("pre004", "C");

        student_1.registered_student_Registration_list().insert(reg_1);
        student_1.registered_student_Registration_list().insert(reg_2);
        student_2.registered_student_Registration_list().insert(reg_3);
        student_2.registered_student_Registration_list().insert(reg_4);
        student_2.registered_student_Registration_list().insert(reg_5);
        student_3.registered_student_Registration_list().insert(reg_6);
        student_3.registered_student_Registration_list().insert(reg_7);
        student_3.registered_student_Registration_list().insert(reg_8);
        student_3.registered_student_Registration_list().insert(reg_9);
        student_3.registered_student_Registration_list().insert(reg_A);
        student_4.registered_student_Registration_list().insert(reg_B);
        student_4.registered_student_Registration_list().insert(reg_C);
        student_5.registered_student_Registration_list().insert(reg_D);
        student_5.registered_student_Registration_list().insert(reg_E);
        student_5.registered_student_Registration_list().insert(reg_F);
        student_5.registered_student_Registration_list().insert(reg_G);

        course_1.registered_course_Registration_list().insert(reg_3);
        course_1.registered_course_Registration_list().insert(reg_8);
        course_1.registered_course_Registration_list().insert(reg_C);
        course_1.registered_course_Registration_list().insert(reg_G);

        course_2.registered_course_Registration_list().insert(reg_1);
        course_2.registered_course_Registration_list().insert(reg_7);
        course_2.registered_course_Registration_list().insert(reg_D);

        course_3.registered_course_Registration_list().insert(reg_4);
        course_3.registered_course_Registration_list().insert(reg_9);
        course_3.registered_course_Registration_list().insert(reg_E);

        course_4.registered_course_Registration_list().insert(reg_2);
        course_4.registered_course_Registration_list().insert(reg_5);
        course_4.registered_course_Registration_list().insert(reg_A);
        course_4.registered_course_Registration_list().insert(reg_B);
        course_4.registered_course_Registration_list().insert(reg_F);

        course_5.registered_course_Registration_list().insert(reg_6);

        course_2.prereq_PreReq_list().insert(prereq_1);
        course_1.course_PreReq_list().insert(prereq_1);

        course_2.prereq_PreReq_list().insert(prereq_2);
        course_3.course_PreReq_list().insert(prereq_2);

        course_2.prereq_PreReq_list().insert(prereq_3);
        course_4.course_PreReq_list().insert(prereq_3);

        course_5.prereq_PreReq_list().insert(prereq_4);
        course_2.course_PreReq_list().insert(prereq_4);
    }
};

TEST_F(test_queries_code, DISABLED_basic_implicit_navigation)
{
    gaia::db::begin_transaction();
    populate_db();
    gaia::db::commit_transaction();

    gaia::rules::initialize_rules_engine();
    // Use the second set of rules.
    gaia::rules::unsubscribe_ruleset("test_tags");

    gaia::db::begin_transaction();
    for (auto& s : Student_t::list())
    {
        if (strcmp(s.Surname(), "Richard") == 0)
        {
            auto sw = s.writer();
            sw.Age = 46;
            sw.update_row();
            break;
        }
    }
    gaia::db::commit_transaction();

    // GAIAPLAT-801
    EXPECT_TRUE(wait_for_rule(g_onupdate_called)) << "OnUpdate(Student) not called";
    EXPECT_EQ(test_error_result_t::e_none, g_onupdate_result) << "OnUpdate failure";
    EXPECT_EQ(g_onupdate_value, 7) << "Rule did not set correct value from implicit query";
}
