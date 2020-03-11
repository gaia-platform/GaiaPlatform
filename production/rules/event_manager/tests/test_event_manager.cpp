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

/**
 * Checkers validate whether the rule was passed the correct context information
 * on invocation
 */ 
class TransactionContextChecker
{
public:    
    TransactionContextChecker()
    {
        reset();
    }

    void set(const transaction_context_t& context)
    {
        set(context.rule_binding.ruleset_name, context.rule_binding.rule_name, 
            context.rule_binding.rule, context.event_type);
    }

    void set(const char * a_ruleset_name,
        const char * a_rule_name,
        gaia_rule_fn a_rule,
        event_type a_type)
    {
        ruleset_name = a_ruleset_name;
        rule_name = a_rule_name;
        rule = a_rule;
        type = a_type;
    }

    void validate(
        const char * a_ruleset_name,
        const char * a_rule_name,
        gaia_rule_fn a_rule,
        event_type a_type)
    { 
        EXPECT_STREQ(ruleset_name, a_ruleset_name);
        EXPECT_STREQ(rule_name, a_rule_name);
        EXPECT_EQ(rule, a_rule);
        EXPECT_EQ(type, a_type);
    }

    void validate_not_called(event_type invalid_type = event_type::row_insert)
    {
        EXPECT_STREQ(ruleset_name, nullptr);
        EXPECT_STREQ(rule_name, nullptr);
        EXPECT_EQ(rule, nullptr);
        EXPECT_EQ(type, invalid_type);
    }

    void reset(event_type invalid_type = event_type::row_insert) 
    {
        ruleset_name = nullptr;
        rule_name = nullptr;
        rule = nullptr;
        // for transaction events, a table event is an illegal type
        // so set it to row_insert unless the parameter is provided
        // by the subclass.
        type = invalid_type;
    }

    // shared between transaction and table context objects
    const char * ruleset_name;
    const char * rule_name;
    gaia_rule_fn rule;
    event_type type;
};
TransactionContextChecker g_transaction_checker;

/**
 * The TableContextChecker adds validation of the gaia_type and gaia_base *
 * that are passed with table context objects.
 */ 
class TableContextChecker : public TransactionContextChecker
{
public:
    TableContextChecker()
    {
        reset();
    }

    void set(const table_context_t& context)
    {
        TransactionContextChecker::set(context.rule_binding.ruleset_name, 
            context.rule_binding.rule_name, context.rule_binding.rule, context.event_type);

        gaia_type = context.gaia_type;
        row = context.row;
    }
    void validate(
        const char * a_ruleset_name,
        const char * a_rule_name,
        gaia_rule_fn a_rule,
        event_type a_type,
        gaia_type_t a_gaia_type,
        gaia_base * a_row) 
    {
        TransactionContextChecker::validate(a_ruleset_name, a_rule_name, a_rule, a_type);
        EXPECT_EQ(gaia_type, a_gaia_type);
        EXPECT_EQ(row, a_row);
        reset();
    }

    void validate_not_called()
    {
        TransactionContextChecker::validate_not_called(event_type::transaction_rollback);
        EXPECT_EQ(gaia_type, 0);
        EXPECT_EQ(row, nullptr);
    }

    void reset() 
    {
        // set the inval event_type to be a transaction event since that will be invalid
        // for all table contexts
        TransactionContextChecker::reset(event_type::transaction_rollback);
        gaia_type = 0;
        row = nullptr;
    }

    // additional data for table context objects
    gaia_type_t gaia_type;
    gaia_base * row;
};
TableContextChecker g_table_checker;

/**
 * Our test object that will serve as the
 * row context sent to table events
 */ 
class TestGaia : public gaia_base
{
public:
    TestGaia() : data(0) {}

    static const gaia_type_t s_gaia_type = 333;

    // rule will set this
    int32_t data;
};


/**
 * Transaction events have no row context so we'll use a global
 * that the rules bound to transaction events will write to.
 */ 
int32_t g_tx_data = 0;

/**
 * Table Rule functions
 * 
 * Make sure the rule operations are commutative since we don't guarantee
 * the order that rules are fired.
 */
const int32_t rule_1_adder = 1;
void rule_1_add_1(const context_base_t * context)
{
    const table_context_t * t = static_cast<const table_context_t *>(context);
    TestGaia * row = static_cast<TestGaia *>(t->row);
    // write date into the class
    row->data += rule_1_adder;
    // record the context that was passed to this rule
    g_table_checker.set(*t);
}

const int32_t rule_2_adder = 100;
void rule_2_add_100(const context_base_t * context)
{
    const table_context_t * t = static_cast<const table_context_t *>(context);
    TestGaia * row = static_cast<TestGaia *>(t->row);
    row->data += rule_2_adder;
    // record the context that was passed to this rule
    g_table_checker.set(*t);
}

/**
 * Transaction Rule functions
 * 
 * Make sure the rule operations are commutative since we don't guarantee
 * the order that rules are fired.
 */
const int32_t rule_3_adder = 1000;
void rule_3_add_1000(const context_base_t * context)
{
    const transaction_context_t * t = static_cast<const transaction_context_t *>(context);
    g_tx_data += rule_3_adder;
    g_transaction_checker.set(*t);
}

const int32_t rule_4_adder = 10000;
void rule_4_add_10000(const context_base_t * context)
{
    const transaction_context_t * t = static_cast<const transaction_context_t *>(context);
    g_tx_data += rule_4_adder;
    g_transaction_checker.set(*t);
}


/**
 *  Google test fixture object.  This class is used by each
 * test case below.  SetUp() is called before each test is run
 * and TearDown() is called after each test case is done.
 */
class EventManagerTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        m_row.data = c_initial;
        g_tx_data = c_initial;
    }

    virtual void TearDown()
    {
        unsubscribe_rules();
        g_table_checker.reset();
        g_transaction_checker.reset();
    }

    void validate_table_rule(
        int32_t value,
        const char * ruleset_name,
        const char * rule_name,
        gaia_rule_fn rule,
        event_type type,
        gaia_type_t gaia_type,
        gaia_base * row) 

    {
        EXPECT_EQ(m_row.data, value);
        g_table_checker.validate(ruleset_name, rule_name, rule, type, gaia_type, row);
    }

    void validate_table_rule_not_called()
    {
        EXPECT_EQ(m_row.data, c_initial);
        g_table_checker.validate_not_called();
    }

    // transaction rule validator (note no gaia_type or row)
    void validate_transaction_rule(
        int32_t value,
        const char * ruleset_name,
        const char * rule_name,
        gaia_rule_fn rule,
        event_type type) 
    {
        EXPECT_EQ(g_tx_data, value);
        g_transaction_checker.validate(ruleset_name, rule_name, rule, type);
    }

    void validate_transaction_rule_not_called()
    {
        EXPECT_EQ(g_tx_data, c_initial);
        g_transaction_checker.validate_not_called();
    }

    void setup_all_rules()
    {
        // setup rule bindings as the following:
        // row_delete: rule1, rule2
        // row_insert: rule2
        // row_update: rule1
        // col_change: none
        // transaction_begin: rule3
        // transaction_commit: rule3, rule4
        // transaction_rollback: rule3, rule4
        EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_delete, m_rule1));
        EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_delete, m_rule2));
        EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_insert, m_rule2));
        EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_update, m_rule1));
        EXPECT_EQ(error_code_t::success, subscribe_transaction_rule(event_type::transaction_begin, m_rule3));
        EXPECT_EQ(error_code_t::success, subscribe_transaction_rule(event_type::transaction_rollback, m_rule3));
        EXPECT_EQ(error_code_t::success, subscribe_transaction_rule(event_type::transaction_commit, m_rule3));
        EXPECT_EQ(error_code_t::success, subscribe_transaction_rule(event_type::transaction_rollback, m_rule4));
        EXPECT_EQ(error_code_t::success, subscribe_transaction_rule(event_type::transaction_commit, m_rule4));
    }


    // table context has data within the Gaia "object"
    TestGaia m_row;
    const int32_t c_initial = 20;

    // rule bindings for use in the test
    rule_binding_t m_rule1{"Ruleset_1", "rule_1_add_1", rule_1_add_1};
    rule_binding_t m_rule2{"Ruleset_1", "rule_2_add_100", rule_2_add_100};
    rule_binding_t m_rule3{"Ruleset_2", "rule_3_add_1000", rule_3_add_1000};
    rule_binding_t m_rule4{"Ruleset_2", "rule_4_add_10000", rule_4_add_10000};

    // transaction context has no additional context
    // but we track that it was called by using g_tx_data;
};


TEST_F(EventManagerTest, LogEventEventModeNotSupported) 
{
    // for Q1, only support immediate mode events
    EXPECT_EQ(error_code_t::event_mode_not_supported, log_transaction_event(event_type::transaction_commit, event_mode::deferred));
}

TEST_F(EventManagerTest, LogEventEventSuccessNoRules) 
{
    // table event
    EXPECT_EQ(error_code_t::success, log_table_event(&m_row, TestGaia::s_gaia_type, event_type::col_change, event_mode::immediate));
    validate_table_rule_not_called();
}

TEST_F(EventManagerTest, LogEventSuccessSingleRuleSingleEvent) {
    int32_t expected_value = m_row.data + rule_1_adder;

    // bind a rule to the row_update event for the TestGaia object
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_update, m_rule1));

    // log an event for row_insert into the table; verify the rule was not fired because it is bound to update, not insert
    EXPECT_EQ(error_code_t::success, log_table_event(&m_row, TestGaia::s_gaia_type, event_type::row_insert, event_mode::immediate));
    validate_table_rule_not_called();

    // now log an update event and verify our rule was fired
    EXPECT_EQ(error_code_t::success, log_table_event(&m_row, TestGaia::s_gaia_type, event_type::row_update, event_mode::immediate));
    validate_table_rule(expected_value, "Ruleset_1", "rule_1_add_1", rule_1_add_1, event_type::row_update, TestGaia::s_gaia_type, &m_row);
}

TEST_F(EventManagerTest, LogTableEventSuccessSingleRuleMultiEvent) 
{
    int32_t expected_value = m_row.data;

    // bind a rule to the row_update event for the TestGaia object
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_update, m_rule1));
    // bind the same rule to the row_insert event for the insert TestGaia object
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_insert, m_rule1));

    // log an event for row_delete into the table; verify the rule was not fired because it is bound to update and insert, not delete
    EXPECT_EQ(error_code_t::success, log_table_event(&m_row, TestGaia::s_gaia_type, event_type::row_delete, event_mode::immediate));
    validate_table_rule_not_called();
    
    // now log an update event and verify our rule was fired
    expected_value += rule_1_adder;
    EXPECT_EQ(error_code_t::success, log_table_event(&m_row, TestGaia::s_gaia_type, event_type::row_update, event_mode::immediate));
    validate_table_rule(expected_value, "Ruleset_1", "rule_1_add_1", rule_1_add_1, event_type::row_update, TestGaia::s_gaia_type, &m_row);

    // now log an insert event and verify our rule was fired
    expected_value += rule_1_adder;
    EXPECT_EQ(error_code_t::success, log_table_event(&m_row, TestGaia::s_gaia_type, event_type::row_insert, event_mode::immediate));
    validate_table_rule(expected_value, "Ruleset_1", "rule_1_add_1", rule_1_add_1, event_type::row_insert, TestGaia::s_gaia_type, &m_row);
}

TEST_F(EventManagerTest, LogTransactionEventSuccessSingleRuleMultiEvent) 
{
    int32_t expected_value = m_row.data;

    // bind the same rule to commit and rollback transaction events
    EXPECT_EQ(error_code_t::success, subscribe_transaction_rule(event_type::transaction_commit, m_rule4));
    EXPECT_EQ(error_code_t::success, subscribe_transaction_rule(event_type::transaction_rollback, m_rule4));
    
    // log an event for transaction_begin; verify the rule was not fired because it is bound to commit and rollback, not begin
    EXPECT_EQ(error_code_t::success, log_transaction_event(event_type::transaction_begin, event_mode::immediate));
    validate_transaction_rule_not_called();
    
    // now log a commit event, verify rule fired
    expected_value += rule_4_adder;
    EXPECT_EQ(error_code_t::success, log_transaction_event(event_type::transaction_commit, event_mode::immediate));
    validate_transaction_rule(expected_value, "Ruleset_2", "rule_4_add_10000", rule_4_add_10000, event_type::transaction_commit);

    // now log a rollback event; verify rule fired
    expected_value += rule_4_adder;
    EXPECT_EQ(error_code_t::success, log_transaction_event(event_type::transaction_rollback, event_mode::immediate));
    validate_transaction_rule(expected_value, "Ruleset_2", "rule_4_add_10000", rule_4_add_10000, event_type::transaction_rollback);
}

TEST_F(EventManagerTest, LogEventSuccessMultiRuleSingleEvent) 
{
    // bind rule1 to the row_delete event for the TestGaia object
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_delete, m_rule1));
    // bind rule2 to row_delete event
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_delete, m_rule2));

    // log an event for col_change into the table; verify the rule was not fired because it is bound to delete
    EXPECT_EQ(error_code_t::success, log_table_event(&m_row, TestGaia::s_gaia_type, event_type::col_change, event_mode::immediate));
    validate_table_rule_not_called();

    // now delete, should invoke both rules 
    int32_t expected_value = m_row.data + (rule_1_adder + rule_2_adder);
    EXPECT_EQ(error_code_t::success, log_table_event(&m_row, TestGaia::s_gaia_type, event_type::row_delete, event_mode::immediate));
    EXPECT_EQ(m_row.data, expected_value);
}

TEST_F(EventManagerTest, LogEventSuccessMultiRuleMultiEvent) 
{
    // setup rule bindings as the following:
    // row_delete: rule1, rule2
    // row_insert: rule2
    // row_update: rule1
    // col_change: none
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_delete, m_rule1));
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_delete, m_rule2));
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_insert, m_rule2));
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_update, m_rule1));

    // log an event for col_change into the table; verify no rules fired
    EXPECT_EQ(error_code_t::success, log_table_event(&m_row, TestGaia::s_gaia_type, event_type::col_change, event_mode::immediate));
    validate_table_rule_not_called();

    // delete should invoke both rules 
    int32_t expected_value = m_row.data + (rule_1_adder + rule_2_adder);
    EXPECT_EQ(error_code_t::success, log_table_event(&m_row, TestGaia::s_gaia_type, event_type::row_delete, event_mode::immediate));
    EXPECT_EQ(m_row.data, expected_value);

    // insert should invoke rule2
    expected_value += rule_2_adder;
    EXPECT_EQ(error_code_t::success, log_table_event(&m_row, TestGaia::s_gaia_type, event_type::row_insert, event_mode::immediate));
    validate_table_rule(expected_value, "Ruleset_1", "rule_2_add_100", rule_2_add_100, event_type::row_insert, TestGaia::s_gaia_type, &m_row);

    // update should invoke rule1
    expected_value += rule_1_adder;
    EXPECT_EQ(error_code_t::success, log_table_event(&m_row, TestGaia::s_gaia_type, event_type::row_update, event_mode::immediate));
    validate_table_rule(expected_value, "Ruleset_1", "rule_1_add_1", rule_1_add_1, event_type::row_update, TestGaia::s_gaia_type, &m_row);
}

TEST_F(EventManagerTest, SubscribeTableRuleSuccess) 
{
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::s_gaia_type, event_type::col_change, m_rule1));

    // valid to bind the same rule to a different events
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_insert, m_rule1));
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_update, m_rule1));
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_delete, m_rule1));

    // be super paranoid and ensure we aren't calling the rule function on subscription
    validate_table_rule_not_called();
}

TEST_F(EventManagerTest, SubscribeTransactionRuleSuccess) 
{
    // valid to bind the same rule to a different events
    EXPECT_EQ(error_code_t::success, subscribe_transaction_rule(event_type::transaction_begin, m_rule3));
    EXPECT_EQ(error_code_t::success, subscribe_transaction_rule(event_type::transaction_commit, m_rule3));
    EXPECT_EQ(error_code_t::success, subscribe_transaction_rule(event_type::transaction_rollback, m_rule3));

    // be super paranoid and ensure we aren't calling the rule function on subscription
    validate_transaction_rule_not_called();
}

TEST_F(EventManagerTest, SubscribeTableInvalidEvent) 
{
    rule_binding_t rb;
    rb.ruleset_name = "Ruleset_1";
    rb.rule = rule_1_add_1;
    rb.rule_name = "rule_1_add_1";

    EXPECT_EQ(error_code_t::invalid_event_type , subscribe_table_rule(TestGaia::s_gaia_type, event_type::transaction_begin, rb));
    EXPECT_EQ(error_code_t::invalid_event_type , subscribe_table_rule(TestGaia::s_gaia_type, event_type::transaction_commit, rb));
    EXPECT_EQ(error_code_t::invalid_event_type , subscribe_table_rule(TestGaia::s_gaia_type, event_type::transaction_rollback, rb));
}

TEST_F(EventManagerTest, SubscribeTransactionInvalidEvent) 
{
    rule_binding_t rb;
    rb.ruleset_name = "Ruleset_2";
    rb.rule = rule_3_add_1000;
    rb.rule_name = "rule_3_add_1000";

    EXPECT_EQ(error_code_t::invalid_event_type , subscribe_transaction_rule(event_type::col_change, rb));
    EXPECT_EQ(error_code_t::invalid_event_type , subscribe_transaction_rule(event_type::row_insert, rb));
    EXPECT_EQ(error_code_t::invalid_event_type , subscribe_transaction_rule(event_type::row_update, rb));
    EXPECT_EQ(error_code_t::invalid_event_type , subscribe_transaction_rule(event_type::row_delete, rb));
}

TEST_F(EventManagerTest, UnsubscribeTableRuleInvalidEvent) 
{
    rule_binding_t rb;
    rb.ruleset_name = "Ruleset_1";
    rb.rule = rule_1_add_1;
    rb.rule_name = "rule_1_add_1";

    // setup
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_delete, rb));

    // validation
    EXPECT_EQ(error_code_t::invalid_event_type , unsubscribe_table_rule(TestGaia::s_gaia_type, event_type::transaction_begin, rb));
    EXPECT_EQ(error_code_t::invalid_event_type , unsubscribe_table_rule(TestGaia::s_gaia_type, event_type::transaction_commit, rb));
    EXPECT_EQ(error_code_t::invalid_event_type , unsubscribe_table_rule(TestGaia::s_gaia_type, event_type::transaction_rollback, rb));
}

TEST_F(EventManagerTest, SubscribeTableInvalidRuleBinding) 
{
    rule_binding_t rb;

    // nothing set    
    EXPECT_EQ(error_code_t::invalid_rule_binding, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_insert, rb));

    rb.ruleset_name = "Ruleset_1";
    EXPECT_EQ(error_code_t::invalid_rule_binding, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_update, rb));

    rb.rule_name = "rule_1_add_1";
    EXPECT_EQ(error_code_t::invalid_rule_binding, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_update, rb));
}

TEST_F(EventManagerTest, UnsubscribeTableRuleInvalidRuleBinding) 
{
    rule_binding_t rb;

    // nothing set
    EXPECT_EQ(error_code_t::invalid_rule_binding, unsubscribe_table_rule(TestGaia::s_gaia_type, event_type::row_delete, rb));

    rb.ruleset_name = "Ruleset_1";
    EXPECT_EQ(error_code_t::invalid_rule_binding, unsubscribe_table_rule(TestGaia::s_gaia_type, event_type::row_delete, rb));

    // rule_name and ruleset_name but no rule
    rb.rule_name = "rule_1_add_1";
    EXPECT_EQ(error_code_t::invalid_rule_binding, unsubscribe_table_rule(TestGaia::s_gaia_type, event_type::row_delete, rb));
}

TEST_F(EventManagerTest, UnsubscribeTransactionRuleInvalidRuleBinding) 
{
    rule_binding_t rb;

    // nothing set
    EXPECT_EQ(error_code_t::invalid_rule_binding, unsubscribe_transaction_rule(event_type::transaction_begin, rb));

    rb.ruleset_name = "Ruleset_2";
    EXPECT_EQ(error_code_t::invalid_rule_binding, unsubscribe_transaction_rule(event_type::transaction_begin, rb));

    // rule_name and ruleset_name but no rule
    rb.rule_name = "rule_4_add_10000";
    EXPECT_EQ(error_code_t::invalid_rule_binding, unsubscribe_transaction_rule(event_type::transaction_begin, rb));
}

TEST_F(EventManagerTest, SubscribeTableDuplicateRule) 
{
    rule_binding_t rb;
    
    rb.ruleset_name = "Ruleset_1";
    rb.rule = rule_1_add_1;
    rb.rule_name = "rule_1_add_1";
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_insert, rb));
    EXPECT_EQ(error_code_t::duplicate_rule, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_insert, rb));

    // Another case of duplicate rule is if we try to bind
    // the same ruleset_name and rule_name to a different rule.
    // Bind to a different event so that it would have been legal
    // if we didn't check for this condition
    rb.rule = rule_2_add_100;
    EXPECT_EQ(error_code_t::duplicate_rule, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_delete, rb));
}

TEST_F(EventManagerTest, UnsubscribeTableRuleRuleNotFound) 
{
    rule_binding_t rb;
    rb.ruleset_name = "Ruleset_1";
    rb.rule_name = "rule_1_add_1";
    rb.rule = rule_1_add_1;
    
    // Rule not there at all with no rules subscribed.
    EXPECT_EQ(error_code_t::rule_not_found, unsubscribe_table_rule(TestGaia::s_gaia_type, event_type::row_update, rb));

    // Setup, subscribe a rule, verify we overwrite the rule_name.
    EXPECT_EQ(error_code_t::success, subscribe_table_rule(TestGaia::s_gaia_type, event_type::row_update, rb));

    // Try to remove the rule from the other table events that we didn't register the rule on.
    EXPECT_EQ(error_code_t::rule_not_found, unsubscribe_table_rule(TestGaia::s_gaia_type, event_type::row_insert, rb));
    EXPECT_EQ(error_code_t::rule_not_found, unsubscribe_table_rule(TestGaia::s_gaia_type, event_type::row_delete, rb));
    EXPECT_EQ(error_code_t::rule_not_found, unsubscribe_table_rule(TestGaia::s_gaia_type, event_type::col_change, rb));

    // Try to remove the rule from a type that we didn't register the rule on (TestGaia::s_gaia_type + 1).
    EXPECT_EQ(error_code_t::rule_not_found, unsubscribe_table_rule(TestGaia::s_gaia_type +1, event_type::row_update, rb));

    // With valid rule registered, now ensure that the rule is not found if we change the rule name
    rb.rule_name = "rule_2_add_100";
    rb.rule = rule_2_add_100;
    EXPECT_EQ(error_code_t::rule_not_found, unsubscribe_table_rule(TestGaia::s_gaia_type, event_type::row_update, rb));

    // ensure we don't find the rule if we change the ruleset_name
    rb.ruleset_name = "Ruleset_2";
    rb.rule_name = "rule_1_add_1";
    rb.rule = rule_1_add_1;
    EXPECT_EQ(error_code_t::rule_not_found, unsubscribe_table_rule(TestGaia::s_gaia_type, event_type::row_update, rb));
}

TEST_F(EventManagerTest, SubscribeTransactionDuplicateRule) 
{
    rule_binding_t rb;
    
    rb.ruleset_name = "Ruleset_2";
    rb.rule = rule_4_add_10000;
    rb.rule_name = "rule_4_add_1000";
    EXPECT_EQ(error_code_t::success, subscribe_transaction_rule(event_type::transaction_commit, rb));
    EXPECT_EQ(error_code_t::duplicate_rule, subscribe_transaction_rule(event_type::transaction_commit, rb));
    
    // Another case of duplicate rule is if we try to bind
    // the same ruleset_name and rule_name to a different rule.
    // Bind to a different event so that it would have been legal
    // if we didn't check for this condition
    rb.rule = rule_3_add_1000;
    EXPECT_EQ(error_code_t::duplicate_rule, subscribe_transaction_rule(event_type::transaction_begin, rb));
}

TEST_F(EventManagerTest, LogEventSuccessMultiRuleMultiEventAllTypes) 
{
    setup_all_rules();

    // rollback should invoke rule3, rule4
    int32_t expected_tx_value = g_tx_data + (rule_3_adder + rule_4_adder);
    EXPECT_EQ(error_code_t::success, log_transaction_event(event_type::transaction_rollback, event_mode::immediate));
    EXPECT_EQ(g_tx_data, expected_tx_value);

    // delete should invoke rule1,rule2
    int32_t expected_row_value = m_row.data + (rule_1_adder + rule_2_adder);
    EXPECT_EQ(error_code_t::success, log_table_event(&m_row, TestGaia::s_gaia_type, event_type::row_delete, event_mode::immediate));
    EXPECT_EQ(m_row.data, expected_row_value);

    // begin should invoke rule3 only
    expected_tx_value += rule_3_adder;
    EXPECT_EQ(error_code_t::success, log_transaction_event(event_type::transaction_begin, event_mode::immediate));
    validate_transaction_rule(expected_tx_value, "Ruleset_2", "rule_3_add_1000", rule_3_add_1000, event_type::transaction_begin);

    // insert should invoke rule2
    expected_row_value += rule_2_adder;
    EXPECT_EQ(error_code_t::success, log_table_event(&m_row, TestGaia::s_gaia_type, event_type::row_insert, event_mode::immediate));
    validate_table_rule(expected_row_value, "Ruleset_1", "rule_2_add_100", rule_2_add_100, event_type::row_insert, TestGaia::s_gaia_type, &m_row);

    // update should invoke rule1
    expected_row_value += rule_1_adder;
    EXPECT_EQ(error_code_t::success, log_table_event(&m_row, TestGaia::s_gaia_type, event_type::row_update, event_mode::immediate));
    validate_table_rule(expected_row_value, "Ruleset_1", "rule_1_add_1", rule_1_add_1, event_type::row_update, TestGaia::s_gaia_type, &m_row);

    // commit should invoke rule3, rule4
    expected_tx_value += (rule_3_adder + rule_4_adder);
    EXPECT_EQ(error_code_t::success, log_transaction_event(event_type::transaction_commit, event_mode::immediate));
    EXPECT_EQ(g_tx_data, expected_tx_value);
}

TEST_F(EventManagerTest, ListRulesNone) 
{
    list_subscriptions_t rules;
    rules.push_back(unique_ptr<subscription_t>(new subscription_t("a", "b", 0, event_type::row_update)));
    EXPECT_EQ(1, rules.size());

    list_subscribed_rules(nullptr, nullptr, nullptr, rules);
    EXPECT_EQ(0, rules.size());
}

TEST_F(EventManagerTest, ListRulesNoFilters) 
{
    list_subscriptions_t rules;
    setup_all_rules();

    list_subscribed_rules(nullptr, nullptr, nullptr, rules);
    EXPECT_EQ(9, rules.size());
}

TEST_F(EventManagerTest, ListRulesRulesetFilter) 
{
    list_subscriptions_t rules;
    setup_all_rules();

    const char * ruleset_filter = "Ruleset_1";

    list_subscribed_rules(ruleset_filter, nullptr, nullptr, rules);
    EXPECT_EQ(4, rules.size());
}

TEST_F(EventManagerTest, ListRulesAllFilters) 
{
    list_subscriptions_t rules;
    setup_all_rules();

    const char * ruleset_filter = "Ruleset_1";
    event_type type_filter = event_type::row_delete;
    gaia_type_t gaia_type_filter = TestGaia::s_gaia_type;

    list_subscribed_rules(ruleset_filter, &gaia_type_filter, &type_filter, rules);
    EXPECT_EQ(2, rules.size());
}

TEST_F(EventManagerTest, ListRulesEventTypeFilter) 
{
    list_subscriptions_t rules;
    setup_all_rules();

    event_type type_filter = event_type::transaction_begin;

    list_subscribed_rules(nullptr, nullptr, &type_filter, rules);
    EXPECT_EQ(1, rules.size());
}

