/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "event_manager_settings.hpp"

#include "gaia/rules/rules.hpp"

using namespace std;

class configuration_error_internal : public gaia::common::configuration_error
{
public:
    explicit configuration_error_internal(const char* key, int value)
    {
        std::stringstream message;
        message << "Invalid value '" << value << "' provided for setting '" << key << "'.";
        m_message = message.str();
    }
};

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
            throw configuration_error_internal(event_manager_settings_t::c_thread_count_key, thread_count);
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
        event_manager_settings_t::c_log_individual_rule_stats_key);
    if (individual_rule_stats_setting)
    {
        settings.enable_rule_stats = *individual_rule_stats_setting;
    }

    auto max_retries_setting = rules_config->get_as<uint32_t>(event_manager_settings_t::c_rule_retry_count_key);
    if (max_retries_setting)
    {
        settings.max_rule_retries = *max_retries_setting;
    }
}
