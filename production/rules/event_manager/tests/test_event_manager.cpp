/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

// do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation
#include "rules.hpp"

using namespace std;
using namespace gaia::events;
using namespace gaia::rules;
using namespace gaia::common;

// Our test row context.
class TestGaia : public gaia_base
{
public:
    TestGaia(int32_t value)
    : data(value) {}
    static const gaia_type_t gaia_type = 333;

    int32_t data;
};

// Make sure rule operations are commutative since we don't guarantee
// the order that rules are fired.

// Rules 1 and 2 will be fired for table rules and therefore
// operate on row data

const int32_t rule_1_adder = 1;
void rule_1_add_1(const context_base_t * context)
{
    const table_context_t * t = static_cast<const table_context_t *>(context);
    TestGaia * row = static_cast<TestGaia *>(t->row);
    row->data += rule_1_adder;
    
}

const int32_t rule_2_adder = 100;
void rule_2_add_100(const context_base_t * context)
{
    const table_context_t * t = static_cast<const table_context_t *>(context);
    TestGaia * row = static_cast<TestGaia *>(t->row);
    row->data += rule_2_adder;
}

// rules 3 and 4 operation on global context since they
// are for the transaction events which don't have
// any row context
int32_t tx_data = 0;

const int32_t rule_3_adder = 1000;
void rule_3_add_1000(const context_base_t * context)
{
    const transaction_context_t * t = static_cast<const transaction_context_t *>(context);
    tx_data += rule_3_adder;
}

const int32_t rule_4_adder = 10000;
void rule_4_add_10000(const context_base_t * context)
{
    const transaction_context_t * t = static_cast<const transaction_context_t *>(context);
    tx_data += rule_4_adder;
}

TEST(EventManagerTest, LogEventEventModeNotSupported) {
    // for Q1, only support immediate mode events
    EXPECT_EQ(error_code_t::event_mode_not_supported, log_transaction_event(event_type::transaction_commit, event_mode::deferred));
}

TEST(EventManagerTest, LogEventEventSuccessNoRules) {
    // table event
    const int32_t value = 20;
    TestGaia row(value);

    EXPECT_EQ(error_code_t::success, log_table_event(&row, TestGaia::gaia_type, event_type::col_change, event_mode::immediate));
    EXPECT_EQ(row.data, value);
}

TEST(EventManagerTest, LogEventSuccessSingleRuleSingleEvent) {
    // table event
    const int32_t value = 20;
    TestGaia row(value);
    rule_binding_t rule1("Ruleset_1", "rule_1_add_1", rule_1_add_1);

    // bind a rule to the row_update event for the TestGaia object
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::gaia_type, event_type::row_update, rule1));

    // log an event for row_insert into the table; verify the rule was not fired because it is bound to update, not insert
    EXPECT_EQ(error_code_t::success, log_table_event(&row, TestGaia::gaia_type, event_type::row_insert, event_mode::immediate));
    EXPECT_EQ(row.data, value);

    // now log an update event and verify our rule was fired
    EXPECT_EQ(error_code_t::success, log_table_event(&row, TestGaia::gaia_type, event_type::row_update, event_mode::immediate));
    EXPECT_EQ(row.data, value + rule_1_adder);

    // cleanup
    EXPECT_EQ(error_code_t::success, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_update, rule1));
}

TEST(EventManagerTest, LogEventSuccessSingleRuleMultiEvent) {
    // table event
    const int32_t value = 20;
    TestGaia row(value);
    rule_binding_t rule1("Ruleset_1", "rule_1_add_1", rule_1_add_1);

    // bind a rule to the row_update event for the TestGaia object
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::gaia_type, event_type::row_update, rule1));
    // bind the same rule to the row_insert event for the insert TestGaia object
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::gaia_type, event_type::row_insert, rule1));

    // log an event for row_delete into the table; verify the rule was not fired because it is bound to update and insert, not delete
    EXPECT_EQ(error_code_t::success, log_table_event(&row, TestGaia::gaia_type, event_type::row_delete, event_mode::immediate));
    EXPECT_EQ(row.data, value);

    // now log an update event and verify our rule was fired
    EXPECT_EQ(error_code_t::success, log_table_event(&row, TestGaia::gaia_type, event_type::row_update, event_mode::immediate));
    EXPECT_EQ(row.data, value + rule_1_adder);

    // now log an insert event and verify our rule was fired
    EXPECT_EQ(error_code_t::success, log_table_event(&row, TestGaia::gaia_type, event_type::row_insert, event_mode::immediate));
    EXPECT_EQ(row.data, value + (rule_1_adder * 2));

    // cleanup
    EXPECT_EQ(error_code_t::success, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_update, rule1));
    EXPECT_EQ(error_code_t::success, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_insert, rule1));
}

TEST(EventManagerTest, LogEventSuccessMultiRuleSingleEvent) {
    // table event
    const int32_t value = 20;
    TestGaia row(value);
    rule_binding_t rule1("Ruleset_1", "rule_1_add_1", rule_1_add_1);
    rule_binding_t rule2("Ruleset_1", "rule_2_add_100", rule_2_add_100);

    // bind rule1 to the row_delete event for the TestGaia object
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::gaia_type, event_type::row_delete, rule1));
    // bind rule2 to row_delete event
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::gaia_type, event_type::row_delete, rule2));

    // log an event for col_change into the table; verify the rule was not fired because it is bound to delete
    EXPECT_EQ(error_code_t::success, log_table_event(&row, TestGaia::gaia_type, event_type::col_change, event_mode::immediate));
    EXPECT_EQ(row.data, value);

    // now delete, should invoke both rules 
    EXPECT_EQ(error_code_t::success, log_table_event(&row, TestGaia::gaia_type, event_type::row_delete, event_mode::immediate));
    EXPECT_EQ(row.data, value + rule_1_adder + rule_2_adder);

    // cleanup
    EXPECT_EQ(error_code_t::success, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_delete, rule2));
    EXPECT_EQ(error_code_t::success, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_delete, rule1));
}

TEST(EventManagerTest, LogEventSuccessMultiRuleMultiEvent) {
    // table event
    const int32_t value = 20;
    TestGaia row(value);
    rule_binding_t rule1("Ruleset_1", "rule_1_add_1", rule_1_add_1);
    rule_binding_t rule2("Ruleset_1", "rule_2_add_100", rule_2_add_100);
    int32_t expected_value = value;

    // setup rule bindings as the following:
    // row_delete: rule1, rule2
    // row_insert: rule2
    // row_update: rule1
    // col_change: none
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::gaia_type, event_type::row_delete, rule1));
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::gaia_type, event_type::row_delete, rule2));
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::gaia_type, event_type::row_insert, rule2));
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::gaia_type, event_type::row_update, rule1));

    // log an event for col_change into the table; verify no rules fired
    EXPECT_EQ(error_code_t::success, log_table_event(&row, TestGaia::gaia_type, event_type::col_change, event_mode::immediate));
    EXPECT_EQ(row.data, expected_value);

    // delete should invoke both rules 
    expected_value += (rule_1_adder + rule_2_adder);
    EXPECT_EQ(error_code_t::success, log_table_event(&row, TestGaia::gaia_type, event_type::row_delete, event_mode::immediate));
    EXPECT_EQ(row.data, expected_value);

    // insert should invoke rule2
    expected_value += rule_2_adder;
    EXPECT_EQ(error_code_t::success, log_table_event(&row, TestGaia::gaia_type, event_type::row_insert, event_mode::immediate));
    EXPECT_EQ(row.data, expected_value);

    // update should invoke rule1
    expected_value += rule_1_adder;
    EXPECT_EQ(error_code_t::success, log_table_event(&row, TestGaia::gaia_type, event_type::row_update, event_mode::immediate));
    EXPECT_EQ(row.data, expected_value);

    // cleanup
    EXPECT_EQ(error_code_t::success, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_delete, rule2));
    EXPECT_EQ(error_code_t::success, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_delete, rule1));
    EXPECT_EQ(error_code_t::success, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_insert, rule2));
    EXPECT_EQ(error_code_t::success, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_update, rule1));
}

TEST(EventManagerTest, SubscribeTableRuleSuccess) {
    rule_binding_t rule("Ruleset_1", "rule_1_add_1", rule_1_add_1);

    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::gaia_type, event_type::col_change, rule));

    // valid to bind the same rule to a different events
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::gaia_type, event_type::row_insert, rule));
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::gaia_type, event_type::row_update, rule));
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::gaia_type, event_type::row_delete, rule));

    // clean up after ourselves which also tests UnsubcribeTableRuleSuccess
    EXPECT_EQ(error_code_t::success, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_delete, rule));
    EXPECT_EQ(error_code_t::success, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_update, rule));
    EXPECT_EQ(error_code_t::success, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_insert, rule));
    EXPECT_EQ(error_code_t::success, unsubscribe_table_rule(TestGaia::gaia_type, event_type::col_change, rule));
}

TEST(EventManagerTest, SubscribeTableInvalidEvent) {
    rule_binding_t rb;
    rb.ruleset_name = "Ruleset_1";
    rb.rule = rule_1_add_1;
    rb.rule_name = "rule_1_add_1";

    EXPECT_EQ(error_code_t::invalid_event_type , subscribe_table_rule(TestGaia::gaia_type, event_type::transaction_begin, rb));
    EXPECT_EQ(error_code_t::invalid_event_type , subscribe_table_rule(TestGaia::gaia_type, event_type::transaction_commit, rb));
    EXPECT_EQ(error_code_t::invalid_event_type , subscribe_table_rule(TestGaia::gaia_type, event_type::transaction_rollback, rb));
}

TEST(EventManagerTest, UnsubscribeTableRuleInvalidEvent) {
    rule_binding_t rb;
    rb.ruleset_name = "Ruleset_1";
    rb.rule = rule_1_add_1;
    rb.rule_name = "rule_1_add_1";

    // setup
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::gaia_type, event_type::row_delete, rb));

    // validation
    EXPECT_EQ(error_code_t::invalid_event_type , unsubscribe_table_rule(TestGaia::gaia_type, event_type::transaction_begin, rb));
    EXPECT_EQ(error_code_t::invalid_event_type , unsubscribe_table_rule(TestGaia::gaia_type, event_type::transaction_commit, rb));
    EXPECT_EQ(error_code_t::invalid_event_type , unsubscribe_table_rule(TestGaia::gaia_type, event_type::transaction_rollback, rb));

    // cleanup
    EXPECT_EQ(error_code_t::success, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_delete, rb));
}

TEST(EventManagerTest, SubscribeTableInvalidRuleBinding) {
    rule_binding_t rb;

    // nothing set    
    EXPECT_EQ(error_code_t::invalid_rule_binding, subscribe_table_rule(TestGaia::gaia_type, event_type::row_insert, rb));

    rb.ruleset_name = "Ruleset_1";
    EXPECT_EQ(error_code_t::invalid_rule_binding, subscribe_table_rule(TestGaia::gaia_type, event_type::row_update, rb));

    rb.rule_name = "rule_1_add_1";
    EXPECT_EQ(error_code_t::invalid_rule_binding, subscribe_table_rule(TestGaia::gaia_type, event_type::row_update, rb));
}

TEST(EventManagerTest, UnsubscribeTableRuleInvalidRuleBinding) {
    rule_binding_t rb;

    // nothing set
    EXPECT_EQ(error_code_t::invalid_rule_binding, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_delete, rb));

    rb.ruleset_name = "Ruleset_1";
    EXPECT_EQ(error_code_t::invalid_rule_binding, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_delete, rb));

    // rule but no ruleset_name
    rb.rule_name = "rule_1_add_1";
    EXPECT_EQ(error_code_t::invalid_rule_binding, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_delete, rb));
}

TEST(EventManagerTest, SubscribeTableDuplicateRule) {
    rule_binding_t rb;
    
    rb.ruleset_name = "Ruleset_1";
    rb.rule = rule_1_add_1;
    rb.rule_name = "rule_1_add_1";
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::gaia_type, event_type::row_insert, rb));
    EXPECT_EQ(error_code_t::duplicate_rule, subscribe_table_rule(TestGaia::gaia_type, event_type::row_insert, rb));

    // Another case of duplicate rule is if we try to bind
    // the same ruleset_name and rule_name to a different rule.
    // Bind to a different event so that it would have been legal
    // if we didn't check for this condition
    rb.rule = rule_2_add_100;
    EXPECT_EQ(error_code_t::duplicate_rule, subscribe_table_rule(TestGaia::gaia_type, event_type::row_delete, rb));

    // cleanup
    rb.rule = rule_1_add_1;
    EXPECT_EQ(error_code_t::success, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_insert, rb));
}

TEST(EventManagerTest, UnsubscribeTableRuleRuleNotFound) {
    rule_binding_t rb;
    rb.ruleset_name = "Ruleset_1";
    rb.rule_name = "rule_1_add_1";
    rb.rule = rule_1_add_1;
    
    // Rule not there at all with no rules subscribed.
    EXPECT_EQ(error_code_t::rule_not_found, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_update, rb));

    // Setup, subscribe a rule, verify we overwrite the rule_name.
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::gaia_type, event_type::row_update, rb));

    // Try to remove the rule from the other table events that we didn't register the rule on.
    EXPECT_EQ(error_code_t::rule_not_found, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_insert, rb));
    EXPECT_EQ(error_code_t::rule_not_found, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_delete, rb));
    EXPECT_EQ(error_code_t::rule_not_found, unsubscribe_table_rule(TestGaia::gaia_type, event_type::col_change, rb));

    // Try to remove the rule from a type that we didn't register the rule on (TestGaia::gaia_type + 1).
    EXPECT_EQ(error_code_t::rule_not_found, unsubscribe_table_rule(TestGaia::gaia_type +1, event_type::row_update, rb));

    // With valid rule registered, now ensure that the rule is not found if we change the rule name
    rb.rule_name = "rule_2_add_100";
    rb.rule = rule_2_add_100;
    EXPECT_EQ(error_code_t::rule_not_found, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_update, rb));

    // ensure we don't find the rule if we change the ruleset_name
    rb.ruleset_name = "Ruleset_2";
    rb.rule_name = "rule_1_add_1";
    rb.rule = rule_1_add_1;
    EXPECT_EQ(error_code_t::rule_not_found, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_update, rb));

    // cleanup
    rb.ruleset_name = "Ruleset_1";
    EXPECT_EQ(error_code_t::success, unsubscribe_table_rule(TestGaia::gaia_type, event_type::row_update, rb));
}
