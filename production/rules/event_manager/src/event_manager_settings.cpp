/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "event_manager_settings.hpp"

#include "exceptions.hpp"
#include "rules.hpp"

using namespace std;

void gaia::rules::event_manager_settings_t::parse_rules_config(
    shared_ptr<cpptoml::table>& root_config,
    event_manager_settings_t& settings)
{
    if (!root_config)
    {
        return;
    }

    shared_ptr<cpptoml::table> rules_config = root_config->get_table(c_rules_section);
    if (!rules_config)
    {
        return;
    }

    auto thread_count_setting = rules_config->get_as<int32_t>(event_manager_settings_t::c_thread_count_key);
    if (thread_count_setting)
    {
        int32_t thread_count = *thread_count_setting;
        if (thread_count == 0)
        {
            throw gaia::common::configuration_error(event_manager_settings_t::c_thread_count_key, thread_count);
        }

        if (thread_count == -1)
        {
            settings.num_background_threads = SIZE_MAX;
        }
        else
        {
            settings.num_background_threads = thread_count;
        }
    }

    auto log_interval_setting = rules_config->get_as<uint32_t>(event_manager_settings_t::c_stats_log_interval_key);
    if (log_interval_setting)
    {
        settings.stats_log_interval = *log_interval_setting;
    }

    auto individual_rule_stats_setting = rules_config->get_as<bool>(
        event_manager_settings_t::c_log_individual_rule_stats);
    if (individual_rule_stats_setting)
    {
        settings.enable_rule_stats = *individual_rule_stats_setting;
    }

    auto num_retries_setting = rules_config->get_as<uint32_t>(event_manager_settings_t::c_rule_retry_count);
    if (num_retries_setting)
    {
        settings.num_rule_retries = *num_retries_setting;
    }
}
