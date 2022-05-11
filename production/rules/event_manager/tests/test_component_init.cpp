/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include <gtest/gtest.h>

#include "gaia/rules/rules.hpp"

#include "gaia_internal/db/db_test_base.hpp"
#include "gaia_internal/rules/rules_config.hpp"
#include "gaia_internal/rules/rules_test_helpers.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::rules;
using namespace std;

class rules__component_init : public db_test_base_t
{
protected:
    // Build up the [Rules] section of a TOML configuration file with rule settings.
    shared_ptr<cpptoml::table> make_rules_config(
        int32_t thread_count,
        int32_t log_interval,
        bool log_rule_stats,
        int32_t retry_count)
    {
        shared_ptr<cpptoml::table> root_config = cpptoml::make_table();
        shared_ptr<cpptoml::table> rules_config = cpptoml::make_table();

        rules_config->insert(event_manager_settings_t::c_thread_count_key, thread_count);
        rules_config->insert(event_manager_settings_t::c_stats_log_interval_key, log_interval);
        rules_config->insert(event_manager_settings_t::c_log_individual_rule_stats_key, log_rule_stats);
        rules_config->insert(event_manager_settings_t::c_rule_retry_count_key, retry_count);

        root_config->insert(event_manager_settings_t::c_rules_section, rules_config);
        return root_config;
    }
};

TEST_F(rules__component_init, component_not_initialized_error)
{
    rule_binding_t dont_care;
    subscription_list_t still_dont_care;
    field_position_list_t ignore;

    EXPECT_THROW(subscribe_rule(0, event_type_t::row_update, ignore, dont_care), initialization_error);
    EXPECT_THROW(unsubscribe_rule(0, event_type_t::row_update, ignore, dont_care), initialization_error);
    EXPECT_THROW(unsubscribe_rules(), initialization_error);
    EXPECT_THROW(list_subscribed_rules(nullptr, nullptr, nullptr, nullptr, still_dont_care), initialization_error);
}

void rule(const rule_context_t*)
{
}

TEST_F(rules__component_init, component_initialized)
{
    rule_binding_t binding("ruleset", "rulename", rule);
    subscription_list_t subscriptions;
    field_position_list_t fields;
    const field_position_t c_position = 10;
    fields.emplace_back(c_position);

    // Custom init disables catalog checks.
    event_manager_settings_t settings;
    settings.enable_catalog_checks = false;
    gaia::rules::test::initialize_rules_engine(settings);

    const gaia_type_t c_gaia_type = 1000;
    subscribe_rule(c_gaia_type, event_type_t::row_update, fields, binding);
    EXPECT_EQ(true, unsubscribe_rule(c_gaia_type, event_type_t::row_update, fields, binding));
    unsubscribe_rules();
    list_subscribed_rules(nullptr, nullptr, nullptr, nullptr, subscriptions);
}

TEST_F(rules__component_init, component_valid_empty_config)
{
    event_manager_settings_t default_settings;
    event_manager_settings_t config_settings;

    shared_ptr<cpptoml::table> root_config;
    event_manager_settings_t::parse_rules_config(root_config, config_settings);

    // No default settings should be overriden.
    EXPECT_EQ(default_settings.num_background_threads, config_settings.num_background_threads);
    EXPECT_EQ(default_settings.enable_catalog_checks, config_settings.enable_catalog_checks);
    EXPECT_EQ(default_settings.enable_rule_stats, config_settings.enable_rule_stats);
    EXPECT_EQ(default_settings.stats_log_interval, config_settings.stats_log_interval);
    EXPECT_EQ(default_settings.max_rule_retries, config_settings.max_rule_retries);
}

// See gaia.conf file for valid settings.
TEST_F(rules__component_init, component_valid_config)
{
    const int32_t c_thread_count = 10;
    const int32_t c_log_interval = 20;
    const int32_t c_retry_count = 42;
    const bool c_enable_rule_stats = true;
    event_manager_settings_t config_settings;

    shared_ptr<cpptoml::table> root_config = make_rules_config(c_thread_count, c_log_interval, c_enable_rule_stats, c_retry_count);
    event_manager_settings_t::parse_rules_config(root_config, config_settings);

    EXPECT_EQ(config_settings.num_background_threads, c_thread_count);
    EXPECT_EQ(config_settings.stats_log_interval, c_log_interval);
    EXPECT_EQ(config_settings.enable_rule_stats, c_enable_rule_stats);
    EXPECT_EQ(config_settings.max_rule_retries, c_retry_count);

    EXPECT_EQ(config_settings.enable_catalog_checks, true);
}

// Ensure the special values of thread count behave correctly.
TEST_F(rules__component_init, component_valid_config_thread_count)
{
    const int32_t c_log_interval = 20;
    const int32_t c_retry_count = 42;
    const bool c_enable_rule_stats = true;
    event_manager_settings_t config_settings;

    // A thread_count of 0 should be disallowed.
    shared_ptr<cpptoml::table> root_config = make_rules_config(0, c_log_interval, c_enable_rule_stats, c_retry_count);
    EXPECT_THROW(event_manager_settings_t::parse_rules_config(root_config, config_settings), configuration_error);

    // A thread_count of -1 should be converted to SIZE_MAX.
    root_config = make_rules_config(-1, c_log_interval, c_enable_rule_stats, c_retry_count);
    event_manager_settings_t::parse_rules_config(root_config, config_settings);
    EXPECT_EQ(config_settings.num_background_threads, SIZE_MAX);
}
