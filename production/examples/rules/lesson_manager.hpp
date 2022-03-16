////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <map>

#include "gaia/direct_access/auto_transaction.hpp"
#include "gaia/logger.hpp"

// Helper class for managing the lessons in the tutorial. There is no
// sample code in this module.

typedef void (*lesson_fn)(void);

class lesson_manager_t
{
public:
    lesson_manager_t() = default;
    void add_lesson(const char* lesson_name, lesson_fn lesson, const char* ruleset_name, bool setup_clinic = false, bool setup_relationships = false);
    bool run_lesson(const std::string& lesson);
    void list_lessons();
    void run_lessons();
    void set_non_interactive(bool non_interactive);

private:
    void enumerate_lessons_(bool run = false);

private:
    struct lesson_plan_t
    {
        lesson_fn lesson;
        const char* ruleset_name;
        bool setup_clinic;
        bool connect_patients_to_doctors;
    };
    std::map<std::string, lesson_plan_t> m_lessons;
};

// Wrapper class around an example to control the scope of
// the transaction and wait for the rules to finish
// executing.
class example_t
{
public:
    static std::atomic<int8_t> s_rule_counter;
    static bool s_non_interactive;

public:
    example_t(const char* description, uint8_t count_rules = 0);
    ~example_t();

private:
    gaia::direct_access::auto_transaction_t m_transaction;
};

// Displays rule information as well as
// tracks the rule counter.
class rule_scope_t
{
public:
    template <typename... T_args>
    rule_scope_t(const char* format, const T_args&... args)
    {
        gaia_log::app().info(format, args...);
    }

    ~rule_scope_t()
    {
        example_t::s_rule_counter--;
    }
};

// Handle tutorial command line arguments.
class args_handler_t
{
public:
    args_handler_t(lesson_manager_t& lesson_manager);
    void parse_and_run(int argc, const char** argv);

private:
    void usage();
    void lesson_not_found(std::string& lesson);
    void invalid_command_line();

private:
    const char* c_help = "--help";
    const char* c_non_interactive = "--non-interactive";
    const char* c_lessons = "--lessons";

    std::string m_command;
    lesson_manager_t& m_lesson_manager;
};
