////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "lesson_manager.hpp"

#include <iostream>

#include "gaia/rules/rules.hpp"

void clean_db();
void setup_clinic(bool connect_patients_to_doctors = false);

// Lesson Manager
void lesson_manager_t::add_lesson(const char* lesson_name, lesson_fn lesson, const char* ruleset_name, bool setup_clinic, bool connect_patients_to_doctors)
{
    m_lessons[lesson_name] = {lesson, ruleset_name, setup_clinic, connect_patients_to_doctors};
}

bool lesson_manager_t::run_lesson(const std::string& lesson)
{
    auto lesson_iter = m_lessons.find(lesson);
    if (lesson_iter == m_lessons.end())
    {
        return false;
    }

    lesson_plan_t plan = lesson_iter->second;

    // By default, all rulesets are subscribed and active after initialization.
    // For this tutorial, only subscribe one ruleset at a time per lesson.
    gaia::rules::unsubscribe_rules();
    clean_db();

    if (plan.setup_clinic)
    {
        setup_clinic(plan.connect_patients_to_doctors);
    }
    gaia::rules::subscribe_ruleset(plan.ruleset_name);
    gaia_log::app().info("\n--- [Lesson '{}'] ---", lesson);
    plan.lesson();

    return true;
}

void lesson_manager_t::enumerate_lessons_(bool run)
{
    for (const auto& pair : m_lessons)
    {
        if (run)
        {
            run_lesson(pair.first);
        }
        else
        {
            std::cout << pair.first << "\n";
        }
    }
}

void lesson_manager_t::list_lessons()
{
    enumerate_lessons_(false);
}

void lesson_manager_t::set_non_interactive(bool non_interactive)
{
    example_t::s_non_interactive = non_interactive;
}

void lesson_manager_t::run_lessons()
{
    enumerate_lessons_(true);
}

// Example wrapper (database side)

std::atomic<int8_t> example_t::s_rule_counter;
bool example_t::s_non_interactive = false;

example_t::example_t(const char* description, uint8_t count_rules)
{
    s_rule_counter = count_rules;
    gaia_log::app().info(description);
}

example_t::~example_t()
{
    const uint32_t c_rule_wait_time_us = 10000;

    m_transaction.commit();

    while (s_rule_counter != 0)
    {
        usleep(10);
    }

    if (!s_non_interactive)
    {
        gaia_log::app().info("Press <enter> to continue...");
        std::cin.get();
    }
    else
    {
        // Give the rule transaction time to commit
        usleep(10);
    }
}

// Command line argument handler
args_handler_t::args_handler_t(lesson_manager_t& lesson_manager)
    : m_lesson_manager(lesson_manager)
{
}

void args_handler_t::parse_and_run(int argc, const char** argv)
{
    m_command = argv[0];

    // Supported command line for 1 arg:
    // ./hospital
    if (argc == 1)
    {
        // By default, run all lessons interactively.
        m_lesson_manager.run_lessons();
        return;
    }

    // Supported command line for 2 args:
    // ./hospital --lessons
    // ./hospital --help
    // ./hospital --non-interactive
    // ./hospital <lesson>
    if (argc == 2)
    {
        std::string arg;
        arg = argv[1];

        if (arg == c_help)
        {
            usage();
        }
        else if (arg == c_lessons)
        {
            m_lesson_manager.list_lessons();
        }
        else if (arg == c_non_interactive)
        {
            m_lesson_manager.set_non_interactive(true);
            m_lesson_manager.run_lessons();
        }
        else if (!m_lesson_manager.run_lesson(arg))
        {
            lesson_not_found(arg);
        }

        return;
    }

    // Supported command lines for 3 args:
    // ./hospital <lesson> --non-interactive
    // ./hospital --non-interactive <lesson>
    if (argc == 3)
    {
        std::string arg1 = argv[1];
        std::string arg2 = argv[2];

        if (arg1 == c_help || arg2 == c_help)
        {
            usage();
            return;
        }

        if (arg1 != c_non_interactive && arg2 != c_non_interactive)
        {
            invalid_command_line();
            return;
        }

        m_lesson_manager.set_non_interactive(true);
        if (arg1 == c_non_interactive)
        {
            if (!m_lesson_manager.run_lesson(arg2))
            {
                lesson_not_found(arg2);
            }
        }
        else
        {
            if (!m_lesson_manager.run_lesson(arg1))
            {
                lesson_not_found(arg1);
            }
        }

        return;
    }

    // If we got here, then just dump out the usage.
    usage();
}

void args_handler_t::usage()
{
    std::cout << "Overview: Gaia Rules Tutorial\n";
    std::cout << "Usage: " << m_command << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  <lesson>             Specifies the lesson to run. If not used, then all lessons are run.\n";
    std::cout << "  " << c_lessons << "            Lists available lessons.\n";
    std::cout << "  " << c_non_interactive << "    Do not pause after each lesson.\n";
    std::cout << "  " << c_help << "               Print this information.\n";
}

void args_handler_t::lesson_not_found(std::string& lesson)
{
    std::cout << "Lesson '" << lesson
              << "' not found.  To see the available lessons, use:" << std::endl
              << m_command << " --lessons" << std::endl;
}

void args_handler_t::invalid_command_line()
{
    std::cout << "A <lesson> can only be combined with the optional '"
              << c_non_interactive << "' flag on the command line."
              << std::endl;
}
