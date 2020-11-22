/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include "gtest/gtest.h"

#include "gaia/db/catalog.hpp"
#include "gaia/direct_access/edc_base.hpp"
#include "gaia/exceptions.hpp"
#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"
#include "db_test_base.hpp"
#include "gaia_catalog.h"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::catalog;
using namespace gaia::direct_access;
using namespace gaia::rules;
using namespace std;

extern "C" void initialize_rules()
{
}

class system_init_test : public db_test_base_t
{
public:
    gaia_type_t load_catalog()
    {
        // Add a dummy type so that the event manager doesn't cry foul when subscribing a rule.
        ddl::field_def_list_t fields;
        fields.emplace_back(make_unique<ddl::data_field_def_t>("id", data_type_t::e_string, 1));
        // The type of the table is the row id of table in the catalog.
        return create_table("system_init_test", fields);
    }

protected:
    // Manage the session ourselves in this test as the
    // gaia::system::initialize() will call begin_session.
    system_init_test()
        : db_test_base_t(true)
    {
    }
};

TEST_F(system_init_test, system_not_initialized_error)
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

TEST_F(system_init_test, system_initialized_valid_conf)
{
    rule_binding_t binding("ruleset", "rulename", rule);
    subscription_list_t subscriptions;

    gaia::system::initialize("./gaia.conf");
    gaia_id_t table_id = load_catalog();
    begin_transaction();
    gaia_type_t type_id = gaia_table_t::get(table_id).type();
    commit_transaction();

    subscribe_rule(type_id, event_type_t::row_update, empty_fields, binding);
    EXPECT_EQ(true, unsubscribe_rule(type_id, event_type_t::row_update, empty_fields, binding));
    unsubscribe_rules();
    list_subscribed_rules(nullptr, nullptr, nullptr, nullptr, subscriptions);

    end_session();
}

TEST_F(system_init_test, system_invalid_conf_path)
{
    EXPECT_THROW(gaia::system::initialize("./bogus_file.conf"), std::exception);
}

TEST_F(system_init_test, system_invalid_conf)
{
    EXPECT_THROW(gaia::system::initialize("./invalid_gaia.conf"), std::exception);
}

TEST_F(system_init_test, system_invalid_setting_conf)
{
    EXPECT_THROW(gaia::system::initialize("./invalid_gaia_setting.conf"), configuration_error);
}
