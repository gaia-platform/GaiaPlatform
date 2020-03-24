/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>


// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.
#include <unordered_map>

#include "gtest/gtest.h"
#include "rules.hpp"

using namespace std;
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

    void set(const char* a_ruleset_name,
        const char* a_rule_name,
        gaia_rule_fn a_rule,
        event_type_t a_type)
    {
        ruleset_name = a_ruleset_name;
        rule_name = a_rule_name;
        rule = a_rule;
        type = a_type;
    }

    void validate(
        const char* a_ruleset_name,
        const char* a_rule_name,
        gaia_rule_fn a_rule,
        event_type_t a_type)
    { 
        EXPECT_STREQ(ruleset_name, a_ruleset_name);
        EXPECT_STREQ(rule_name, a_rule_name);
        EXPECT_EQ(rule, a_rule);
        EXPECT_EQ(type, a_type);
    }

    void validate_not_called(event_type_t invalid_type = event_type_t::row_insert)
    {
        EXPECT_STREQ(ruleset_name, nullptr);
        EXPECT_STREQ(rule_name, nullptr);
        EXPECT_EQ(rule, nullptr);
        EXPECT_EQ(type, invalid_type);
    }

    void reset(event_type_t invalid_type = event_type_t::row_insert) 
    {
        ruleset_name = nullptr;
        rule_name = nullptr;
        rule = nullptr;
        // For transaction events, a table event is an illegal type
        // so set it to row_insert unless the parameter is provided
        // by the subclass.  I.e., for the table sub class will
        // pass this in as a transaction event type.
        type = invalid_type;
    }

    // These fields are passed context
    // for both table and transaction events.
    const char* ruleset_name;
    const char* rule_name;
    gaia_rule_fn rule;
    event_type_t type;
};
TransactionContextChecker g_transaction_checker;

/**
 * The TableContextChecker adds validation of the gaia_type and gaia_base*
 * that are passed as part of the table context to rule invocations.
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
        const char* a_ruleset_name,
        const char* a_rule_name,
        gaia_rule_fn a_rule,
        event_type_t a_type,
        gaia_type_t a_gaia_type,
        gaia_base* a_row) 
    {
        TransactionContextChecker::validate(a_ruleset_name, a_rule_name, a_rule, a_type);
        EXPECT_EQ(gaia_type, a_gaia_type);
        EXPECT_EQ(row, a_row);
        reset();
    }

    void validate_not_called()
    {
        TransactionContextChecker::validate_not_called(event_type_t::transaction_rollback);
        EXPECT_EQ(gaia_type, 0);
        EXPECT_EQ(row, nullptr);
    }

    void reset() 
    {
        // Set the invalid event_type to be a transaction event since that will be invalid
        // for all table contexts.
        TransactionContextChecker::reset(event_type_t::transaction_rollback);
        gaia_type = 0;
        row = nullptr;
    }

    // Additional data for table context objects over and above
    // context for transaction objects.
    gaia_type_t gaia_type;
    gaia_base* row;
};
TableContextChecker g_table_checker;

/**
 * Our test object that will serve as the
 * row context sent to table events.
 */ 
class TestGaia : public gaia_base
{
public:
    TestGaia() : data(0) {}

    static const gaia_type_t s_gaia_type = 333;

    // rule will set this
    int32_t data;
};

// Only to test gaia type filters on the
// list_subscribed_rules api.
class TestGaia2 : public gaia_base
{
public:    
    static const gaia_type_t s_gaia_type = 444;
};

/**
 * Transaction events have no row context so we'll use a global
 * variable that the rules bound to transaction events will write to
 * to verify that the rule was executed.
 */ 
int32_t g_tx_data = 0;

/**
 * Applications must provide an implementation for initialize_rules().
 * This function is called on construction of the singleton event
 * manager instance.  If this function is not called then every test
 * will fail below because the condition is checked on TearDown() of
 * ever test case in the test fixture.
 */
 uint32_t g_initialize_rules_called = 0;

 extern "C"
 void initialize_rules()
 {
     ++g_initialize_rules_called;
 }
 
/**
 * Table Rule functions.
 * 
 * Make sure the rule operations are commutative since we don't guarantee
 * the order that rules are fired.  Table rules write to the passed in "row"
 * data.
 */
const int32_t rule1_adder = 1;
void rule1_add_1(const context_base_t* context)
{
    const table_context_t* t = static_cast<const table_context_t*>(context);
    TestGaia * row = static_cast<TestGaia *>(t->row);
    // write date into the class
    row->data += rule1_adder;
    // record the context that was passed to this rule
    g_table_checker.set(*t);
}

const int32_t rule2_adder = 100;
void rule2_add_100(const context_base_t* context)
{
    const table_context_t* t = static_cast<const table_context_t*>(context);
    TestGaia * row = static_cast<TestGaia *>(t->row);
    row->data += rule2_adder;
    // record the context that was passed to this rule
    g_table_checker.set(*t);
}

/**
 * Transaction Rule functions
 * 
 * Make sure the rule operations are commutative since we don't guarantee
 * the order that rules are fired.  Transaction rules don't have any
 * associated context so they write to a global variable.
 */
const int32_t rule3_adder = 1000;
void rule3_add_1000(const context_base_t* context)
{
    const transaction_context_t* t = static_cast<const transaction_context_t*>(context);
    g_tx_data += rule3_adder;
    g_transaction_checker.set(*t);
}

const int32_t rule4_adder = 10000;
void rule4_add_10000(const context_base_t* context)
{
    const transaction_context_t* t = static_cast<const transaction_context_t*>(context);
    g_tx_data += rule4_adder;
    g_transaction_checker.set(*t);
}

/**
 * Following constructs are used to verify the list_subscribed_rules API
 * returns the correct rules based on the filter criteria to the API.  It
 * also is used to do table-driven tests on differetn rule binding configurations.
 */ 
typedef std::unordered_map<string, subscription_t> map_subscriptions_t;

static constexpr char ruleset1_name[] = "Ruleset_1";
static constexpr char ruleset2_name[] = "Ruleset_2";
static constexpr char rule1_name[] = "rule1_add_1";
static constexpr char rule2_name[] = "rule2_add_100";
static constexpr char rule3_name[] = "rule3_add_1000";
static constexpr char rule4_name[] = "rule4_add_10000";

/**
 * This type enables subscription_t as well as rule_binding_t
 * to be built off of it.  It is used to provide expected results
 * to validation functions of the test fixture and enable a table
 * driven testing approach.
 */ 
struct rule_decl_t{
    subscription_t sub;
    gaia_rule_fn fn;
};


/**
 * Setup rule bindings as follows:
 * 
 * <TestGaia Object Type>
 * row_delete: rule1, rule2
 * row_insert: rule2
 * row_update: rule1
 *        
 * <TestGaia2 Object Type>
 * col_change: rule1, rule2
 * row_insert: rule3, rule4
 *
 * transaction_begin: rule3
 * transaction_commit: rule3, rule4
 * transaction_rollback: rule3, rule4
 */ 
static constexpr rule_decl_t s_rule_decl[] = {
    {{ruleset1_name, rule1_name, TestGaia::s_gaia_type, event_type_t::row_delete}, rule1_add_1},
    {{ruleset1_name, rule2_name, TestGaia::s_gaia_type, event_type_t::row_delete}, rule2_add_100},
    {{ruleset1_name, rule2_name, TestGaia::s_gaia_type, event_type_t::row_insert}, rule2_add_100},
    {{ruleset1_name, rule1_name, TestGaia::s_gaia_type, event_type_t::row_update}, rule1_add_1},
    {{ruleset2_name, rule3_name, TestGaia2::s_gaia_type, event_type_t::row_insert}, rule3_add_1000},
    {{ruleset2_name, rule4_name, TestGaia2::s_gaia_type, event_type_t::row_insert}, rule4_add_10000},
    {{ruleset1_name, rule1_name, TestGaia2::s_gaia_type, event_type_t::col_change}, rule1_add_1},
    {{ruleset1_name, rule2_name, TestGaia2::s_gaia_type, event_type_t::col_change}, rule2_add_100},
    {{ruleset2_name, rule3_name, 0, event_type_t::transaction_begin}, rule3_add_1000},
    {{ruleset2_name, rule3_name, 0, event_type_t::transaction_commit}, rule3_add_1000},
    {{ruleset2_name, rule4_name, 0, event_type_t::transaction_commit}, rule4_add_10000},
    {{ruleset2_name, rule3_name, 0, event_type_t::transaction_rollback}, rule3_add_1000},
    {{ruleset2_name, rule4_name, 0, event_type_t::transaction_rollback}, rule4_add_10000}
};
static constexpr int s_rule_decl_len = sizeof(s_rule_decl)/sizeof(s_rule_decl[0]);


/**
 * Google test fixture object.  This class is used by each
 * test case below.  SetUp() is called before each test is run
 * and TearDown() is called after each test case is done.
 */
class event_manager_test : public ::testing::Test
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

        // This expectation verifies that the caller provided
        // initialize_rules function was called exactly once by
        // the event_manager_t singleton.
        EXPECT_EQ(1, g_initialize_rules_called);
    }

    void validate_table_rule(
        int32_t value,
        const char* ruleset_name,
        const char* rule_name,
        gaia_rule_fn rule,
        event_type_t type,
        gaia_type_t gaia_type,
        gaia_base* row) 

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
        const char* ruleset_name,
        const char* rule_name,
        gaia_rule_fn rule,
        event_type_t type) 
    {
        EXPECT_EQ(g_tx_data, value);
        g_transaction_checker.validate(ruleset_name, rule_name, rule, type);
    }

    map_subscriptions_t get_expected_subscriptions(
        const char* ruleset_filter,
        const gaia_type_t* gaia_type_filter,
        const event_type_t* event_type_filter)
    {
        map_subscriptions_t expected_subscriptions;

        for (int i = 0; i < s_rule_decl_len; ++i) 
        {
            const rule_decl_t& decl = s_rule_decl[i];
            if (ruleset_filter) {
                if (0 != strcmp(ruleset_filter, decl.sub.ruleset_name)) {
                    continue;
                }
            }

            if (gaia_type_filter) {
                if (*gaia_type_filter != decl.sub.gaia_type) {
                    continue;
                }
            }

            if (event_type_filter) {
                if (*event_type_filter != decl.sub.type) {
                    continue;
                }
            }

            expected_subscriptions.insert(pair<string, subscription_t>(
                make_subscription_key(decl.sub),
                {decl.sub.ruleset_name, decl.sub.rule_name, 
                decl.sub.gaia_type, decl.sub.type}));
        }

        return expected_subscriptions;
    }

    std::string make_subscription_key(const subscription_t& sub)
    {
        string key = sub.ruleset_name;
        key.append(sub.rule_name);
        key.append(to_string(sub.gaia_type));
        key.append(to_string((int)sub.type));
        return key;
    }

    void validate_transaction_rule_not_called()
    {
        EXPECT_EQ(g_tx_data, c_initial);
        g_transaction_checker.validate_not_called();
    }

    void validate_rule_list(const list_subscriptions_t& subscriptions, 
        const map_subscriptions_t& expected_subscriptions)
    {
        EXPECT_EQ(subscriptions.size(), expected_subscriptions.size());

        for(auto sub_it = subscriptions.begin(); sub_it != subscriptions.end(); ++sub_it)
        {
            // make a key,
            // verify it's found in the map
            string key = make_subscription_key(**sub_it);
            auto map_it = expected_subscriptions.find(key);
            EXPECT_FALSE(map_it == expected_subscriptions.end());
            assert_lists_are_equal(**sub_it, map_it->second);
        }
    }

    void assert_lists_are_equal(const subscription_t& a, const subscription_t& b)
    {
        EXPECT_STREQ(a.ruleset_name, b.ruleset_name);
        EXPECT_STREQ(a.ruleset_name, b.ruleset_name);
        EXPECT_EQ(a.gaia_type, b.gaia_type);
        EXPECT_EQ(a.type, b.type);
    }

    void setup_all_rules()
    {
        for (int i=0; i < s_rule_decl_len; ++i)
        {
            const rule_decl_t& decl = s_rule_decl[i];
            rule_binding_t binding;
            binding.ruleset_name = decl.sub.ruleset_name;
            binding.rule_name = decl.sub.rule_name;
            binding.rule = decl.fn;

            event_type_t event = decl.sub.type;
            gaia_type_t gaia_type = decl.sub.gaia_type;

            // only table subscriptions have a gaia_type
            if (gaia_type) 
            {
                subscribe_table_rule(gaia_type, event, binding);
            }
            else 
            {
                subscribe_transaction_rule(event, binding);
            }
        }
    }

    // For debugging only; don't clutter up test output.
    void dump_rules(const list_subscriptions_t& subscriptions)
    {
        static const char* s_event_name[] = {
            "transaction_begin",
            "transaction_commit",
            "tranasction_rollback",
            "col_change",
            "row_update",
            "row_insert",
            "row_delete"
        };

        for(auto sub_it = subscriptions.begin(); sub_it != subscriptions.end(); ++sub_it)
        {
            printf("%s::%s, %lu, %s\n",(*sub_it)->ruleset_name, (*sub_it)->rule_name, (*sub_it)->gaia_type, 
                s_event_name[(int)(*sub_it)->type]);
        }
    }

    // Table context has data within the Gaia "object".
    TestGaia m_row;
    const int32_t c_initial = 20;

    // Rule bindings for use in the test.
    rule_binding_t m_rule1{ruleset1_name, rule1_name, rule1_add_1};
    rule_binding_t m_rule2{ruleset1_name, rule2_name, rule2_add_100};
    rule_binding_t m_rule3{ruleset2_name, rule3_name, rule3_add_1000};
    rule_binding_t m_rule4{ruleset2_name, rule4_name, rule4_add_10000};
};

TEST_F(event_manager_test, log_event_mode_not_supported) 
{
    // For Q1, only support immediate mode events.
    EXPECT_THROW(log_transaction_event(event_type_t::transaction_commit, event_mode_t::deferred), mode_not_supported);
    EXPECT_THROW(log_table_event(nullptr, TestGaia2::s_gaia_type, event_type_t::transaction_rollback, event_mode_t::deferred), mode_not_supported);
}

TEST_F(event_manager_test, invalid_event_type) 
{
    EXPECT_THROW(log_transaction_event(event_type_t::row_insert, event_mode_t::immediate), invalid_event_type);
    EXPECT_THROW(log_table_event(nullptr, TestGaia2::s_gaia_type, event_type_t::transaction_rollback, event_mode_t::immediate), invalid_event_type);
}

TEST_F(event_manager_test, log_event_no_rules) 
{
    EXPECT_EQ(false, log_table_event(&m_row, TestGaia::s_gaia_type, event_type_t::col_change, event_mode_t::immediate));
    validate_table_rule_not_called();
}

TEST_F(event_manager_test, log_table_event_single_event_single_rule) {
    int32_t expected_value = m_row.data + rule1_adder;

    // Subscribe to update.
    subscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_update, m_rule1);

    // Log insert; verify the rule was not fired because it is bound to update, not insert.
    EXPECT_EQ(false, log_table_event(&m_row, TestGaia::s_gaia_type, event_type_t::row_insert, event_mode_t::immediate));
    validate_table_rule_not_called();

    // Log update
    EXPECT_EQ(true, log_table_event(&m_row, TestGaia::s_gaia_type, event_type_t::row_update, event_mode_t::immediate));
    validate_table_rule(expected_value, ruleset1_name, rule1_name, rule1_add_1, event_type_t::row_update, TestGaia::s_gaia_type, &m_row);
}

TEST_F(event_manager_test, log_table_event_single_rule_multi_event) 
{
    int32_t expected_value = m_row.data;

    // Bind same rule to update and insert
    subscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_update, m_rule1);
    subscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_insert, m_rule1);

    // Log delete; verify no rules fired.
    EXPECT_EQ(false, log_table_event(&m_row, TestGaia::s_gaia_type, event_type_t::row_delete, event_mode_t::immediate));
    validate_table_rule_not_called();
    
    // Log update followed by insert and verify the rule gets called twice.
    expected_value += rule1_adder;
    EXPECT_EQ(true, log_table_event(&m_row, TestGaia::s_gaia_type, event_type_t::row_update, event_mode_t::immediate));
    validate_table_rule(expected_value, ruleset1_name, rule1_name, rule1_add_1, event_type_t::row_update, TestGaia::s_gaia_type, &m_row);

    expected_value += rule1_adder;
    EXPECT_EQ(true, log_table_event(&m_row, TestGaia::s_gaia_type, event_type_t::row_insert, event_mode_t::immediate));
    validate_table_rule(expected_value, ruleset1_name, rule1_name, rule1_add_1, event_type_t::row_insert, TestGaia::s_gaia_type, &m_row);
}

TEST_F(event_manager_test, log_transaction_event_single_rule_multi_event) 
{
    int32_t expected_value = m_row.data;

    // Bind same rule to commit and rollback.
    subscribe_transaction_rule(event_type_t::transaction_commit, m_rule4);
    subscribe_transaction_rule(event_type_t::transaction_rollback, m_rule4);
    
    // Log begin; verify no rules fired.
    EXPECT_EQ(false, log_transaction_event(event_type_t::transaction_begin, event_mode_t::immediate));
    validate_transaction_rule_not_called();
    
    // Log commit and update; verify rule gets called twice.
    expected_value += rule4_adder;
    EXPECT_EQ(true, log_transaction_event(event_type_t::transaction_commit, event_mode_t::immediate));
    validate_transaction_rule(expected_value, ruleset2_name, rule4_name, rule4_add_10000, event_type_t::transaction_commit);

    expected_value += rule4_adder;
    EXPECT_EQ(true, log_transaction_event(event_type_t::transaction_rollback, event_mode_t::immediate));
    validate_transaction_rule(expected_value, ruleset2_name, rule4_name, rule4_add_10000, event_type_t::transaction_rollback);
}

TEST_F(event_manager_test, log_table_event_multi_rule_single_event) 
{
    // Bind two rules to the same event.
    subscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_delete, m_rule1);
    subscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_delete, m_rule2);

    // Log a col_change event.  Verify no rules fired.
    EXPECT_EQ(false, log_table_event(&m_row, TestGaia::s_gaia_type, event_type_t::col_change, event_mode_t::immediate));
    validate_table_rule_not_called();

    // Verify logging a delete event fires both rules.
    int32_t expected_value = m_row.data + (rule1_adder + rule2_adder);
    EXPECT_EQ(true, log_table_event(&m_row, TestGaia::s_gaia_type, event_type_t::row_delete, event_mode_t::immediate));
    EXPECT_EQ(m_row.data, expected_value);
}

TEST_F(event_manager_test, log_event_multi_rule_multi_event) 
{
    // See comment on definition of s_rule_decl for which
    // events are setup.
    setup_all_rules();

    // Be super paranoid and ensure we aren't calling the rule function on subscription.
    validate_table_rule_not_called();
    validate_transaction_rule_not_called();

    // Log event for TestGaia::col_change.
    EXPECT_EQ(false, log_table_event(&m_row, TestGaia::s_gaia_type, event_type_t::col_change, event_mode_t::immediate));
    validate_table_rule_not_called();

    // Log event TestGaia::delete to invoke rule1 and rule2.
    int32_t expected_value = m_row.data + (rule1_adder + rule2_adder);
    EXPECT_EQ(true, log_table_event(&m_row, TestGaia::s_gaia_type, event_type_t::row_delete, event_mode_t::immediate));
    EXPECT_EQ(m_row.data, expected_value);

    // Unsubscribe rule1 from delete now; log delete; verify only rule2 gets fired.
    expected_value += rule2_adder;
    EXPECT_EQ(true, unsubscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_delete, m_rule1));
    EXPECT_EQ(true, log_table_event(&m_row, TestGaia::s_gaia_type, event_type_t::row_delete, event_mode_t::immediate));
    validate_table_rule(expected_value, ruleset1_name, rule2_name, rule2_add_100, event_type_t::row_delete, TestGaia::s_gaia_type, &m_row);

    // Insert should invoke rule2.
    expected_value += rule2_adder;
    EXPECT_EQ(true, log_table_event(&m_row, TestGaia::s_gaia_type, event_type_t::row_insert, event_mode_t::immediate));
    validate_table_rule(expected_value, ruleset1_name, rule2_name, rule2_add_100, event_type_t::row_insert, TestGaia::s_gaia_type, &m_row);

    // Update should invoke rule1.
    expected_value += rule1_adder;
    EXPECT_EQ(true, log_table_event(&m_row, TestGaia::s_gaia_type, event_type_t::row_update, event_mode_t::immediate));
    validate_table_rule(expected_value, ruleset1_name, rule1_name, rule1_add_1, event_type_t::row_update, TestGaia::s_gaia_type, &m_row);

    // Rollback should invoke rule3, rule4.
    int32_t expected_tx_value = g_tx_data + (rule3_adder + rule4_adder);
    EXPECT_EQ(true, log_transaction_event(event_type_t::transaction_rollback, event_mode_t::immediate));
    EXPECT_EQ(g_tx_data, expected_tx_value);

    // Begin should invoke rule3 only.
    expected_tx_value += rule3_adder;
    EXPECT_EQ(true, log_transaction_event(event_type_t::transaction_begin, event_mode_t::immediate));
    validate_transaction_rule(expected_tx_value, ruleset2_name, rule3_name, rule3_add_1000, event_type_t::transaction_begin);
   
    // commit should invoke rule3, rule4
    expected_tx_value += (rule3_adder + rule4_adder);
    EXPECT_EQ(true, log_transaction_event(event_type_t::transaction_commit, event_mode_t::immediate));
    EXPECT_EQ(g_tx_data, expected_tx_value);
}

TEST_F(event_manager_test, subscribe_table_rule_invalid_event) 
{
    rule_binding_t rb;
    rb.ruleset_name = ruleset1_name;
    rb.rule = rule1_add_1;
    rb.rule_name = rule1_name;

    EXPECT_THROW(subscribe_table_rule(TestGaia::s_gaia_type, event_type_t::transaction_begin, rb), invalid_event_type);
    EXPECT_THROW(subscribe_table_rule(TestGaia::s_gaia_type, event_type_t::transaction_commit, rb), invalid_event_type);
    EXPECT_THROW(subscribe_table_rule(TestGaia::s_gaia_type, event_type_t::transaction_rollback, rb), invalid_event_type);
}

TEST_F(event_manager_test, subscribe_transaction_rule_invalid_event) 
{
    rule_binding_t rb;
    rb.ruleset_name = ruleset2_name;
    rb.rule = rule3_add_1000;
    rb.rule_name = rule3_name;

    EXPECT_THROW(subscribe_transaction_rule(event_type_t::col_change, rb), invalid_event_type);
    EXPECT_THROW(subscribe_transaction_rule(event_type_t::row_insert, rb), invalid_event_type);
    EXPECT_THROW(subscribe_transaction_rule(event_type_t::row_update, rb), invalid_event_type);
    EXPECT_THROW(subscribe_transaction_rule(event_type_t::row_delete, rb), invalid_event_type);
}


TEST_F(event_manager_test, unsubscribe_table_rule_invalid_event) 
{
    rule_binding_t rb;
    rb.ruleset_name = ruleset1_name;
    rb.rule = rule1_add_1;
    rb.rule_name = rule1_name;

    EXPECT_THROW(unsubscribe_table_rule(TestGaia::s_gaia_type, event_type_t::transaction_begin, rb), invalid_event_type);
    EXPECT_THROW(unsubscribe_table_rule(TestGaia::s_gaia_type, event_type_t::transaction_commit, rb), invalid_event_type);
    EXPECT_THROW(unsubscribe_table_rule(TestGaia::s_gaia_type, event_type_t::transaction_rollback, rb), invalid_event_type);
}

TEST_F(event_manager_test, subscribe_table_rule_invalid_rule_binding) 
{
    rule_binding_t rb;

    // Empty binding.
    EXPECT_THROW(subscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_insert, rb), invalid_rule_binding);

    // No rule_name or rule set.
    rb.ruleset_name = ruleset1_name;
    EXPECT_THROW(subscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_update, rb), invalid_rule_binding);

    // No rule set.
    rb.rule_name = rule1_name;
    EXPECT_THROW(subscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_update, rb), invalid_rule_binding);
}


TEST_F(event_manager_test, unsubscribe_table_rule_invalid_rule_binding) 
{
    rule_binding_t rb;

    // Empty binding
    EXPECT_THROW(unsubscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_delete, rb), invalid_rule_binding);

    // No rule_name or rule set.
    rb.ruleset_name = ruleset1_name;
    EXPECT_THROW(unsubscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_delete, rb), invalid_rule_binding);

    // No rule set.
    rb.rule_name = rule1_name;
    EXPECT_THROW(unsubscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_delete, rb), invalid_rule_binding);
}

TEST_F(event_manager_test, unsubscribe_transaction_rule_invalid_rule_binding) 
{
    rule_binding_t rb;

    // Empty binding
    EXPECT_THROW(unsubscribe_transaction_rule(event_type_t::transaction_begin, rb), invalid_rule_binding);

    // No rule or rule name set.
    rb.ruleset_name = ruleset2_name;
    EXPECT_THROW(unsubscribe_transaction_rule(event_type_t::transaction_begin, rb),invalid_rule_binding);

    // No rule.
    rb.rule_name = rule4_name;
    EXPECT_THROW(unsubscribe_transaction_rule(event_type_t::transaction_begin, rb), invalid_rule_binding);
}

TEST_F(event_manager_test, unsubscribe_table_rule_duplicate_rule) 
{
    rule_binding_t rb;
    rb.ruleset_name = ruleset1_name;
    rb.rule = rule1_add_1;
    rb.rule_name = rule1_name;

    subscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_insert, rb);
    EXPECT_THROW(subscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_insert, rb), duplicate_rule);

    // Another case of duplicate rule is if we try to bind
    // the same ruleset_name and rule_name to a different rule.
    // Bind to a different event so that it would have been legal
    // if we didn't check for this condition
    rb.rule = rule2_add_100;
    EXPECT_THROW(subscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_delete, rb), duplicate_rule);
}

TEST_F(event_manager_test, unsubscribe_table_rule_rule_not_found) 
{
    rule_binding_t rb;
    rb.ruleset_name = ruleset1_name;
    rb.rule_name = rule1_name;
    rb.rule = rule1_add_1;
    
    // Rule not there at all with no rules subscribed.
    EXPECT_EQ(false, unsubscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_update, rb));

    // Subscribe a valid rule.
    subscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_update, rb);

    // Try to remove the rule from the other table events that we didn't register the rule on.
    EXPECT_EQ(false, unsubscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_insert, rb));
    EXPECT_EQ(false, unsubscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_delete, rb));
    EXPECT_EQ(false, unsubscribe_table_rule(TestGaia::s_gaia_type, event_type_t::col_change, rb));

    // Try to remove the rule from a type that we didn't register the rule on
    EXPECT_EQ(false, unsubscribe_table_rule(TestGaia2::s_gaia_type, event_type_t::row_update, rb));

    // With valid rule registered, now ensure that the rule is not found if we change the rule name
    rb.rule_name = rule2_name;
    rb.rule = rule2_add_100;
    EXPECT_EQ(false, unsubscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_update, rb));

    // Ensure we don't find the rule if we change the ruleset_name
    rb.ruleset_name = ruleset2_name;
    rb.rule_name = rule1_name;
    rb.rule = rule1_add_1;
    EXPECT_EQ(false, unsubscribe_table_rule(TestGaia::s_gaia_type, event_type_t::row_update, rb));
}

TEST_F(event_manager_test, subscribe_transaction_rule_duplicate_rule) 
{
    rule_binding_t rb;
    rb.ruleset_name = ruleset2_name;
    rb.rule = rule4_add_10000;
    rb.rule_name = "rule4_add_1000";

    subscribe_transaction_rule(event_type_t::transaction_commit, rb);
    EXPECT_THROW(subscribe_transaction_rule(event_type_t::transaction_commit, rb), duplicate_rule);
    
    // Another case of duplicate rule is if we try to bind
    // the same ruleset_name and rule_name to a different rule.
    // Bind to a different event so that it would have been legal
    // if we didn't check for this condition
    rb.rule = rule3_add_1000;
    EXPECT_THROW(subscribe_transaction_rule(event_type_t::transaction_begin, rb), duplicate_rule);
}


TEST_F(event_manager_test, ListRulesNone) 
{
    list_subscriptions_t rules;

    // Verify that the list_subscribed_rules api clears the subscription list the caller
    // passes in.
    rules.push_back(unique_ptr<subscription_t>(new subscription_t({"a", "b", 0, event_type_t::row_update})));
    EXPECT_EQ(1, rules.size());

    list_subscribed_rules(nullptr, nullptr, nullptr, rules);
    EXPECT_EQ(0, rules.size());
}

TEST_F(event_manager_test, ListRulesNoFilters) 
{
    list_subscriptions_t rules;
    setup_all_rules();

    list_subscribed_rules(nullptr, nullptr, nullptr, rules);
    validate_rule_list(rules, get_expected_subscriptions(nullptr, nullptr, nullptr));
}

TEST_F(event_manager_test, ListRulesRulesetFilter) 
{
    list_subscriptions_t rules;
    setup_all_rules();

    const char* ruleset_filter = ruleset1_name;
    list_subscribed_rules(ruleset_filter, nullptr, nullptr, rules);
    validate_rule_list(rules, get_expected_subscriptions(ruleset_filter, nullptr, nullptr));

    ruleset_filter = ruleset2_name;
    list_subscribed_rules(ruleset_filter, nullptr, nullptr, rules);
    validate_rule_list(rules, get_expected_subscriptions(ruleset_filter, nullptr, nullptr));
}

TEST_F(event_manager_test, ListRulesEventTypeFilter) 
{
    list_subscriptions_t rules;
    setup_all_rules();

    event_type_t event_filter = event_type_t::transaction_begin;
    list_subscribed_rules(nullptr, nullptr, &event_filter, rules);
    validate_rule_list(rules, get_expected_subscriptions(nullptr, nullptr, &event_filter));

    event_filter = event_type_t::row_insert;
    list_subscribed_rules(nullptr, nullptr, &event_filter, rules);
    validate_rule_list(rules, get_expected_subscriptions(nullptr, nullptr, &event_filter));
}

TEST_F(event_manager_test, ListRulesGaiaTypeFilter) 
{
    list_subscriptions_t rules;
    setup_all_rules();

    gaia_type_t gaia_type_filter = TestGaia2::s_gaia_type;
    list_subscribed_rules(nullptr, &gaia_type_filter, nullptr, rules);
    validate_rule_list(rules, get_expected_subscriptions(
        nullptr, &gaia_type_filter, nullptr));

    gaia_type_filter = TestGaia::s_gaia_type;
    list_subscribed_rules(nullptr, &gaia_type_filter, nullptr, rules);
    validate_rule_list(rules, get_expected_subscriptions(
        nullptr, &gaia_type_filter, nullptr));
}

TEST_F(event_manager_test, ListRulesAllFilters) 
{
    list_subscriptions_t rules;
    setup_all_rules();

    const char* ruleset_filter = ruleset1_name;
    gaia_type_t gaia_type_filter = TestGaia::s_gaia_type;
    event_type_t event_filter = event_type_t::row_delete;
    list_subscribed_rules(ruleset_filter, &gaia_type_filter, &event_filter, rules);
    validate_rule_list(rules, get_expected_subscriptions(ruleset_filter, 
        &gaia_type_filter, &event_filter));

    ruleset_filter = ruleset2_name;
    event_filter = event_type_t::col_change;
    gaia_type_filter = TestGaia2::s_gaia_type;
    list_subscribed_rules(ruleset_filter, &gaia_type_filter, &event_filter, rules);
    validate_rule_list(rules, get_expected_subscriptions(ruleset_filter, 
        &gaia_type_filter, &event_filter));
}
