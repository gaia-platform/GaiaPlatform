/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

// do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation
#include "rules.hpp"

#include "gtest/gtest.h"

using namespace std;
using namespace gaia::events;
using namespace gaia::rules;
using namespace gaia::common;

// Our test row context.
class TestGaia : public gaia_base
{
public:
    static const gaia_type_t gaia_type = 333;
    int32_t i = 20;
};

// Our rule in Test_Set_1;
void rule_1_abcdefg(const context_base_t * context)
{
    const table_context_t * t = static_cast<const table_context_t *>(context);
    TestGaia * row = static_cast<TestGaia *>(t->row);
    EXPECT_EQ(20, row->i);
}

TEST(EventManagerTest, EventApi) {
    // table event
    TestGaia row;

    EXPECT_EQ(false, log_table_event(&row, event_type::col_change, event_mode::deferred));
    EXPECT_EQ(false, log_transaction_event(event_type::transaction_commit, event_mode::deferred));
}

TEST(EventManagerTest, RuleApi) {
    // table event
    TestGaia row;

    rule_binding_t rb;
    rb.ruleset_name = "Test_Set_1";
    rb.rule = rule_1_abcdefg;

    EXPECT_EQ(false, subscribe_table_rule(TestGaia::gaia_type, event_type::col_change, rb));
    EXPECT_EQ(true, subscribe_transaction_rule(event_type::transaction_commit, rb));
    EXPECT_EQ(false, unsubscribe_table_rule(TestGaia::gaia_type, event_type::col_change, rb));
    EXPECT_EQ(false, unsubscribe_transaction_rule(event_type::col_change, rb));

    std::vector<const char *> rule_names;
    list_subscribed_rules(nullptr, nullptr, nullptr, rule_names);
    
    gaia_type_t t = TestGaia::gaia_type;
    event_type et = event_type::col_change;
    list_subscribed_rules(rb.ruleset_name, &t, &et, rule_names);
}
