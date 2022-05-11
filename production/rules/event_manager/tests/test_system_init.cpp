/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include <gtest/gtest.h>

#include "gaia/common.hpp"
#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/db/db.hpp"
#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_addr_book.h"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::catalog;
using namespace gaia::direct_access;
using namespace gaia::rules;
using namespace std;

void rule(const rule_context_t*)
{
}

class rules__system_init : public db_catalog_test_base_t
{
public:
    void verify_initialized()
    {
        rule_binding_t binding("ruleset", "rulename", rule);
        subscription_list_t subscriptions;

        subscribe_rule(gaia::addr_book::employee_t::s_gaia_type, event_type_t::row_update, empty_fields, binding);
        EXPECT_EQ(true, unsubscribe_rule(gaia::addr_book::employee_t::s_gaia_type, event_type_t::row_update, empty_fields, binding));
        unsubscribe_rules();
        list_subscribed_rules(nullptr, nullptr, nullptr, nullptr, subscriptions);
    }

protected:
    // Manage the session ourselves in this test as the
    // gaia::system::initialize() will call begin_session.
    rules__system_init()
        : db_catalog_test_base_t("addr_book.ddl", false)
    {
    }

    static void SetUpTestSuite()
    {
        gaia_log::initialize({});

        server_instance_config_t server_conf = server_instance_config_t::get_new_instance_config();
        server_conf.instance_name = c_default_instance_name;

        s_server_instance = server_instance_t{server_conf};

        // Make the instance name the default, so that calls to begin_session()
        // will automatically connect to that instance.
        config::session_options_t session_options;
        session_options.db_instance_name = s_server_instance.instance_name();
        session_options.skip_catalog_integrity_check = s_server_instance.skip_catalog_integrity_check();
        config::set_default_session_options(session_options);

        s_server_instance.start();
        s_server_instance.wait_for_init();
    }
};

TEST_F(rules__system_init, not_initialized_error)
{
    rule_binding_t dont_care;
    subscription_list_t still_dont_care;
    field_position_list_t ignore;

    EXPECT_THROW(subscribe_rule(c_invalid_gaia_type, event_type_t::row_update, ignore, dont_care), initialization_error);
    EXPECT_THROW(unsubscribe_rule(c_invalid_gaia_type, event_type_t::row_update, ignore, dont_care), initialization_error);
    EXPECT_THROW(unsubscribe_rules(), initialization_error);
    EXPECT_THROW(list_subscribed_rules(nullptr, nullptr, nullptr, nullptr, still_dont_care), initialization_error);
}

TEST_F(rules__system_init, valid_conf_both_args)
{
    // Should be a no-op if the system has not been initialized
    gaia::system::shutdown();

    gaia::system::initialize("./gaia.conf", "./gaia_log.conf");
    verify_initialized();
    gaia::system::shutdown();
}

TEST_F(rules__system_init, valid_conf)
{
    // Should be a no-op if the system has not been initialized
    gaia::system::shutdown();

    gaia::system::initialize();
    verify_initialized();
    gaia::system::shutdown();
}

TEST_F(rules__system_init, valid_conf_gaia_arg)
{
    // Should be a no-op if the system has not been initialized
    gaia::system::shutdown();

    gaia::system::initialize("./gaia.conf");
    verify_initialized();
    gaia::system::shutdown();
}

TEST_F(rules__system_init, valid_conf_logger_arg)
{

    // Should be a no-op if the system has not been initialized
    gaia::system::shutdown();

    gaia::system::initialize(nullptr, "./gaia_log.conf");
    verify_initialized();
    gaia::system::shutdown();
}

TEST_F(rules__system_init, invalid_gaia_conf_path)
{
    EXPECT_THROW(gaia::system::initialize("./bogus_file.conf"), configuration_error);
}

TEST_F(rules__system_init, invalid_gaia_log_conf_path)
{
    EXPECT_THROW(gaia::system::initialize(nullptr, "./bogus_file.conf"), configuration_error);
}

TEST_F(rules__system_init, invalid_conf_path)
{
    EXPECT_THROW(gaia::system::initialize("./bogus_file.conf", "./bogus_file.conf"), configuration_error);
}

TEST_F(rules__system_init, invalid_conf)
{
    // Let through the more informative parse error
    EXPECT_THROW(gaia::system::initialize("./invalid_gaia.conf"), std::exception);
}

TEST_F(rules__system_init, invalid_setting_conf)
{
    EXPECT_THROW(gaia::system::initialize("./invalid_gaia_setting.conf"), configuration_error);
}
