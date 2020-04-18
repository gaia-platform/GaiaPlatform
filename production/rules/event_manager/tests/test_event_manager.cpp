/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include <unordered_map>
#include "gtest/gtest.h"
#include "rules.hpp"
#include "gaia_system.hpp"
#include "event_log_gaia_generated.h"

using namespace std;
using namespace gaia::rules;
using namespace gaia::common;

/**
 * The rule_context_checker_t validates whethe the rule was passed the
 * correct context information on invocation.
 */ 
class rule_context_checker_t
{
public:
    rule_context_checker_t()
    {
        reset();
    }

    void set(const rule_context_t* context)
    {
        ruleset_name = context->rule_binding.ruleset_name;
        rule_name = context->rule_binding.rule_name;
        rule = context->rule_binding.rule;
        event_type = context->event_type;
        gaia_type = context->gaia_type;
        row = context->event_context;
        event_source = context->event_source;
    }

    void validate(
        const char* a_ruleset_name,
        const char* a_rule_name,
        gaia_rule_fn a_rule,
        event_type_t a_event_type,
        gaia_type_t a_gaia_type,
        gaia_base_t* a_row,
        const char* a_source)
    {
        EXPECT_STREQ(ruleset_name, a_ruleset_name);
        EXPECT_STREQ(rule_name, a_rule_name);
        EXPECT_EQ(rule, a_rule);
        EXPECT_EQ(event_type, a_event_type);
        EXPECT_EQ(gaia_type, a_gaia_type);
        EXPECT_EQ(row, a_row);
        EXPECT_STREQ(event_source.c_str(), a_source);

        // UNDONE: Dax, why the reset again?
        reset();
    }

    void validate_not_called(event_type_t invalid_type = event_type_t::field_read)
    {
        EXPECT_STREQ(ruleset_name, nullptr);
        EXPECT_STREQ(rule_name, nullptr);
        EXPECT_EQ(rule, nullptr);
        EXPECT_EQ(event_type, invalid_type);
        EXPECT_EQ(gaia_type, 0);
        EXPECT_EQ(row, nullptr);
        EXPECT_EQ(true, event_source.empty());
    }

        
    // Set the invalid event_type to be a field event since that will be invalid
    // for all database contexts.
    void reset(event_type_t invalid_type = event_type_t::field_read) 
    {
        ruleset_name = nullptr;
        rule_name = nullptr;
        rule = nullptr;
        event_type = invalid_type;
        gaia_type = 0;
        row = nullptr;
        event_source.clear();
    }

    // Additional data for table context objects over and above
    // context for transaction objects.
    gaia_type_t gaia_type;
    gaia_base_t* row;
    const char* ruleset_name;
    const char* rule_name;
    gaia_rule_fn rule;
    event_type_t event_type;
    string event_source;
};
rule_context_checker_t g_context_checker;

/**
 * Our test object that will serve as the
 * row context sent to table events.
 */ 
class TestGaia : public gaia_base_t
{
public:
    TestGaia() 
    : gaia_base_t("TestGaia")
    , data(0) {}

    static const gaia_type_t s_gaia_type;
    gaia_type_t gaia_type() override
    {
        return s_gaia_type;
    }

    void reset(bool) override {}

    // rule will set this
    int32_t data;
};
const gaia_type_t TestGaia::s_gaia_type = 333;

// Only to test gaia type filters on the
// list_subscribed_rules api.
class TestGaia2 : public gaia_base_t
{
public:
    TestGaia2() 
    : gaia_base_t("TestGaia2")
    , data(0) {}

    static const gaia_type_t s_gaia_type;
    gaia_type_t gaia_type() override
    {
        return s_gaia_type;
    }

    void reset(bool) override {}
    
    // rule will set this
    int32_t data;
};
const gaia_type_t TestGaia2::s_gaia_type = 444;

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
 * Following constructs are used to verify the list_subscribed_rules API
 * returns the correct rules based on the filter criteria to the API.  It
 * also is used to do table-driven tests on different rule binding configurations.
 */ 
typedef std::unordered_map<string, subscription_t> map_subscriptions_t;
static constexpr char ruleset1_name[] = "Ruleset_1";
static constexpr char ruleset2_name[] = "Ruleset_2";
static constexpr char ruleset3_name[] = "Ruleset_3";
static constexpr char rule1_name[] = "rule1_add_1";
static constexpr char rule2_name[] = "rule2_add_100";
static constexpr char rule3_name[] = "rule3_add_1000";
static constexpr char rule4_name[] = "rule4_add_10000";
static constexpr char rule5_name[] = "rule5_add_100000";
static constexpr char rule6_name[] = "rule6_add_1000000";
static constexpr char rule7_name[] = "rule7_add_10000000";
static constexpr char rule8_name[] = "rule8_add_100000000";
 
/**
 * Table Rule functions.
 * 
 * Make sure the rule operations are commutative since we don't guarantee
 * the order that rules are fired.  Table rules write to the passed in "row"
 * data.
 */
const int32_t rule1_adder = 1;
void rule1_add_1(const rule_context_t* context)
{
    TestGaia * row = static_cast<TestGaia *>(context->event_context);

    // write date into the class
    row->data += rule1_adder;
    // record the context that was passed to this rule
    g_context_checker.set(context);
}

const int32_t rule2_adder = 100;
void rule2_add_100(const rule_context_t* context)
{
    TestGaia * row = static_cast<TestGaia *>(context->event_context);
    row->data += rule2_adder;
    // record the context that was passed to this rule
    g_context_checker.set(context);
}

/**
 * Transaction Rule functions
 * 
 * Make sure the rule operations are commutative since we don't guarantee
 * the order that rules are fired.  Transaction rules don't have any
 * associated context so they write to a global variable.
 */
const int32_t rule3_adder = 1000;
void rule3_add_1000(const rule_context_t* context)
{
    g_tx_data += rule3_adder;
    g_context_checker.set(context);
}

const int32_t rule4_adder = 10000;
void rule4_add_10000(const rule_context_t* context)
{
    g_tx_data += rule4_adder;
    g_context_checker.set(context);
}

/**
 * Setup for forward chaining Rule functions.  Separated from above tests
 * to increase clarity.  Forward chaining is a feature
 * that allows a rule to do an action that results in the
 * firing of another rule.  In Q1 we only allow forward
 * chaining to different events and prevent recursion (either
 * immediate or in a cycle).
 */

bool is_rule_subscribed(
    const char* ruleset_filter, 
    const gaia_type_t gaia_type, 
    const event_type_t event_type,
    const char* field_filter)
{
    subscription_list_t subscriptions;
    gaia_type_t gaia_type_filter = gaia_type;
    event_type_t event_type_filter = event_type;

    list_subscribed_rules(ruleset_filter, &gaia_type_filter, &event_type_filter, field_filter, subscriptions);

    return (subscriptions.size() == 1);
}

/**
 * Cheater variable to ensure our reentrancy logic
 * correctly deals with cycles.
 */
bool g_forward_chain_cycle = false;
const int32_t rule5_adder = 100000;
const int32_t rule6_adder = 1000000;
const int32_t rule7_adder = 10000000;
const int32_t rule8_adder = 100000000;

/**
 * Rule 5 handles an TestGaia::update event
 * Attempts to forward chain to TestGaia::update (disallowed: same gaia_type and event_type)
 * [Rule 6] Forward chains to TestGaia2::update (allowed: different gaia_type)
 * [Rule 7] Forward chains to TestGaia::insert (allowed: different event_type)
 */
void rule5_add_100000(const rule_context_t* context)
{
    TestGaia * row = static_cast<TestGaia *>(context->event_context);
    row->data += rule5_adder;
    g_context_checker.set(context);

    // Verify this rule is bound to the correct type and event.
    EXPECT_EQ(TestGaia::s_gaia_type, context->gaia_type);
    EXPECT_EQ(event_type_t::row_update, context->event_type);

    // Disallow reentrant event call.
    EXPECT_EQ(false, log_database_event(row, context->event_type, event_mode_t::immediate));

    // Allow event call on different gaia_type.
    TestGaia2 obj2;
    int32_t expected_value = obj2.data + rule6_adder;
    bool expect_rule_fired = is_rule_subscribed(ruleset3_name, TestGaia2::s_gaia_type, context->event_type, nullptr);
    EXPECT_EQ(expect_rule_fired, log_database_event(&obj2, context->event_type, event_mode_t::immediate));
    if (expect_rule_fired)
    {
        EXPECT_EQ(obj2.data, expected_value);
    }

    // Allow event call on different event_type.
    expected_value = row->data + rule7_adder;
    expect_rule_fired = is_rule_subscribed(ruleset3_name, TestGaia::s_gaia_type, event_type_t::row_insert, nullptr);
    EXPECT_EQ(expect_rule_fired, log_database_event(row, event_type_t::row_insert, event_mode_t::immediate));
    if (expect_rule_fired)
    {
        EXPECT_EQ(row->data, expected_value);
    }
}

/**
 * Rule 6 handles TestGaia2::update
 * [Rule 8] Forward chains to Transaction::commit event (allowed: different event class)
 */ 
void rule6_add_1000000(const rule_context_t* context)
{
    TestGaia2 * row = static_cast<TestGaia2 *>(context->event_context);
    row->data += rule6_adder;
    g_context_checker.set(context);

    // Verify this rule is bound to the correct type and event.
    EXPECT_EQ(TestGaia2::s_gaia_type, context->gaia_type);
    EXPECT_EQ(event_type_t::row_update, context->event_type);

    // Allow different event class (transaction event, not table event)
    EXPECT_EQ(true, log_database_event(nullptr, event_type_t::transaction_commit, event_mode_t::immediate));
}

/**
 * Rule 7 handles TestGaia::insert
 */
void rule7_add_10000000(const rule_context_t* context)
{
    TestGaia * row = static_cast<TestGaia *>(context->event_context);
    row->data += rule7_adder;
    g_context_checker.set(context);

    // Verify this rule is bound to the correct type and event.
    EXPECT_EQ(TestGaia::s_gaia_type, context->gaia_type);
    EXPECT_EQ(event_type_t::row_insert, context->event_type);
}

/**
 * Rule 8 handles Transaction::commit
 * Attempts to forward chain to 
 * [Rule 5] TestGaia::Update 
 * 
 * Disallowed if following forward chain exists:
 * [Rule 5] TestGaia::Update -> 
 * [Rule 6] TestGaia2::Update -> 
 * [Rule 8] Transaction::Commit -> 
 * [Rule 5] TestGaia::Update
 * 
 * Allowed otherwise (TestGaia::Update event not in the call
 * hierarchy).
 */
void rule8_add_100000000(const rule_context_t* context)
{
    g_tx_data += rule8_adder;
    g_context_checker.set(context);

    // Verify this rule is bound to the correct event.
    EXPECT_EQ(event_type_t::transaction_commit, context->event_type);

    TestGaia row;

    // We expect the rule to be fired only if it is subscribed
    // and we are not in a forward chain cycle.
    bool expect_rule_fired = is_rule_subscribed(ruleset3_name, TestGaia::s_gaia_type, event_type_t::row_update, nullptr);
    if (g_forward_chain_cycle)
    {
        expect_rule_fired = false;
    }
    EXPECT_EQ(expect_rule_fired, log_database_event(&row, event_type_t::row_update, event_mode_t::immediate));
}

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
 * field_write [first_name, last_name]: rule1, rule2
 * row_insert: rule3, rule4
 *
 * transaction_begin: rule3
 * transaction_commit: rule3, rule4
 * transaction_rollback: rule3, rule4
 */ 
static constexpr rule_decl_t s_rule_decl[] = {
    {{ruleset1_name, rule1_name, TestGaia::s_gaia_type, event_type_t::row_delete, nullptr}, rule1_add_1},
    {{ruleset1_name, rule2_name, TestGaia::s_gaia_type, event_type_t::row_delete, nullptr}, rule2_add_100},
    {{ruleset1_name, rule2_name, TestGaia::s_gaia_type, event_type_t::row_insert, nullptr}, rule2_add_100},
    {{ruleset1_name, rule1_name, TestGaia::s_gaia_type, event_type_t::row_update, nullptr}, rule1_add_1},
    {{ruleset2_name, rule3_name, TestGaia2::s_gaia_type, event_type_t::row_insert, nullptr}, rule3_add_1000},
    {{ruleset2_name, rule4_name, TestGaia2::s_gaia_type, event_type_t::row_insert, nullptr}, rule4_add_10000},
    {{ruleset1_name, rule1_name, TestGaia2::s_gaia_type, event_type_t::field_write, "first_name"}, rule1_add_1},
    {{ruleset1_name, rule2_name, TestGaia2::s_gaia_type, event_type_t::field_write, "last_name"}, rule2_add_100},
    {{ruleset2_name, rule3_name, 0, event_type_t::transaction_begin, nullptr}, rule3_add_1000},
    {{ruleset2_name, rule3_name, 0, event_type_t::transaction_commit, nullptr}, rule3_add_1000},
    {{ruleset2_name, rule4_name, 0, event_type_t::transaction_commit, nullptr}, rule4_add_10000},
    {{ruleset2_name, rule3_name, 0, event_type_t::transaction_rollback, nullptr}, rule3_add_1000},
    {{ruleset2_name, rule4_name, 0, event_type_t::transaction_rollback, nullptr}, rule4_add_10000}
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
        gaia_base_t::begin_transaction();
        m_row.data = c_initial;
        g_tx_data = c_initial;
    }

    virtual void TearDown()
    {
        gaia_base_t::commit_transaction();
        unsubscribe_rules();
        g_context_checker.reset();

        // This expectation verifies that the caller provided
        // initialize_rules function was called exactly once by
        // the event_manager_t singleton.
        EXPECT_EQ(1, g_initialize_rules_called);
    }

    void validate_rule(
        int32_t value,
        const char* ruleset_name,
        const char* rule_name,
        gaia_rule_fn rule,
        event_type_t type,
        gaia_type_t gaia_type,
        gaia_base_t* row,
        const char* source) 

    {
        if (row) 
        {
            EXPECT_EQ(m_row.data, value);
        }
        else
        {
            EXPECT_EQ(g_tx_data, value);
        }
        
        g_context_checker.validate(ruleset_name, rule_name, rule, type, gaia_type, row, source);
    }

    void validate_rule_not_called()
    {
        EXPECT_EQ(m_row.data, c_initial);
        g_context_checker.validate_not_called();
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
                decl.sub.gaia_type, decl.sub.type, decl.sub.field_name}));
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

    void validate_rule_list(const subscription_list_t& subscriptions, 
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

            if (decl.sub.field_name)
            {
                field_list_t fields;
                fields.insert(decl.sub.field_name);
                subscribe_field_rule(gaia_type, event, fields, binding);
            }
            else
            {
                subscribe_database_rule(gaia_type, event, binding);
            }
        }
    }

    // event log table helpers
    typedef unique_ptr<Event_log> log_entry_t;
    uint64_t clear_event_log()
    {
      uint64_t rows_cleared = 0;
      log_entry_t entry(Event_log::get_first());
      while(entry)
      {
          entry->delete_row();
          entry.reset(Event_log::get_first());
          rows_cleared++;
      }
      return rows_cleared;
    }

    void verify_event_log_row(const Event_log& row, uint64_t gaia_type, 
        event_type_t event_type, event_mode_t event_mode, const char * event_source, gaia_id_t id, bool rules_fired)
    {
        EXPECT_EQ(row.gaia_type_id(), gaia_type);
        EXPECT_EQ(row.event_type(), (uint32_t) event_type);
        EXPECT_EQ(row.event_mode(), (uint8_t) event_mode);
        EXPECT_STREQ(row.event_source(), event_source);
        EXPECT_EQ(row.gaia_id(), id);
        EXPECT_EQ(row.rules_fired(), rules_fired);
    }


    // Table context has data within the Gaia "object".
    TestGaia m_row;
    // Table context has data within the Gaia2 "object".
    TestGaia2 m_row2;

    const int32_t c_initial = 20;

    // Rule bindings for use in the test.
    rule_binding_t m_rule1{ruleset1_name, rule1_name, rule1_add_1};
    rule_binding_t m_rule2{ruleset1_name, rule2_name, rule2_add_100};
    rule_binding_t m_rule3{ruleset2_name, rule3_name, rule3_add_1000};
    rule_binding_t m_rule4{ruleset2_name, rule4_name, rule4_add_10000};
    rule_binding_t m_rule5{ruleset3_name, rule5_name, rule5_add_100000};
    rule_binding_t m_rule6{ruleset3_name, rule6_name, rule6_add_1000000};
    rule_binding_t m_rule7{ruleset3_name, rule7_name, rule7_add_10000000};
    rule_binding_t m_rule8{ruleset3_name, rule8_name, rule8_add_100000000};
};

TEST_F(event_manager_test, log_event_mode_not_supported) 
{
    // For Q1, only support immediate mode events.
    EXPECT_THROW(log_database_event(nullptr, event_type_t::transaction_commit, event_mode_t::deferred), mode_not_supported);
    EXPECT_THROW(log_database_event(nullptr, event_type_t::transaction_rollback, event_mode_t::deferred), mode_not_supported);
}

TEST_F(event_manager_test, invalid_event_type) 
{
    EXPECT_THROW(log_database_event(nullptr, event_type_t::field_read, event_mode_t::immediate), invalid_event_type);
    EXPECT_THROW(log_database_event(nullptr, event_type_t::field_write, event_mode_t::immediate), invalid_event_type);
}

TEST_F(event_manager_test, invalid_context)
{
    EXPECT_THROW(log_database_event(&m_row, event_type_t::transaction_begin, event_mode_t::immediate), invalid_context);
    EXPECT_THROW(log_database_event(nullptr, event_type_t::row_update, event_mode_t::immediate), invalid_context);
    EXPECT_THROW(log_field_event(nullptr, nullptr, event_type_t::field_write, event_mode_t::immediate), invalid_context);
    EXPECT_THROW(log_field_event(nullptr, "last_name", event_type_t::field_write, event_mode_t::immediate), invalid_context);
    EXPECT_THROW(log_field_event(&m_row, nullptr, event_type_t::field_write, event_mode_t::immediate), invalid_context);
}

TEST_F(event_manager_test, log_event_no_rules) 
{
    EXPECT_EQ(false, log_database_event(&m_row, event_type_t::row_delete, event_mode_t::immediate));
    validate_rule_not_called();
}

TEST_F(event_manager_test, log_database_event_single_event_single_rule) {
    int32_t expected_value = m_row.data + rule1_adder;

    // Subscribe to update.
    subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_update, m_rule1);

    // Log insert; verify the rule was not fired because it is bound to update, not insert.
    EXPECT_EQ(false, log_database_event(&m_row, event_type_t::row_insert, event_mode_t::immediate));
    validate_rule_not_called();

    // Log update
    EXPECT_EQ(true, log_database_event(&m_row, event_type_t::row_update, event_mode_t::immediate));
    validate_rule(expected_value, ruleset1_name, rule1_name, rule1_add_1, 
        event_type_t::row_update, TestGaia::s_gaia_type, &m_row, m_row.gaia_typename());
}


TEST_F(event_manager_test, log_field_event_single_event_single_rule) {
    // Ensure we have field level granularity.
    field_list_t fields;

    // Binding to an empty field list won't fire any rules.
    subscribe_field_rule(TestGaia::s_gaia_type, event_type_t::field_write, fields, m_rule1);
    EXPECT_EQ(false, log_field_event(&m_row, "last_name", event_type_t::field_write, event_mode_t::immediate));
    validate_rule_not_called();
    // Verify that no rules were subscribed for an emtpy field list.
    EXPECT_EQ(false, unsubscribe_field_rule(TestGaia::s_gaia_type, event_type_t::field_write, fields, m_rule1));

    fields.insert("last_name");
    subscribe_field_rule(TestGaia::s_gaia_type, event_type_t::field_write, fields, m_rule1);
    
    int32_t expected_value = m_row.data + rule1_adder;
    EXPECT_EQ(true, log_field_event(&m_row, "last_name", event_type_t::field_write, event_mode_t::immediate));
    validate_rule(expected_value, ruleset1_name, rule1_name, rule1_add_1, 
        event_type_t::field_write, TestGaia::s_gaia_type, &m_row, "TestGaia.last_name");
}

TEST_F(event_manager_test, log_field_event_multi_event_single_rule) {
    // Ensure we have field level granularity.
    field_list_t fields;

    // Rule 1 will fire on any writes to "last_name" or "first_name"
    fields.insert("last_name");
    fields.insert("first_name");
    subscribe_field_rule(TestGaia::s_gaia_type, event_type_t::field_write, fields, m_rule1);

    int32_t expected_value = m_row.data + rule1_adder;
    EXPECT_EQ(true, log_field_event(&m_row, "last_name", event_type_t::field_write, event_mode_t::immediate));
    validate_rule(expected_value, ruleset1_name, rule1_name, rule1_add_1, 
        event_type_t::field_write, TestGaia::s_gaia_type, &m_row, "TestGaia.last_name");

    expected_value += rule1_adder;
    EXPECT_EQ(true, log_field_event(&m_row, "first_name", event_type_t::field_write, event_mode_t::immediate));        
    validate_rule(expected_value, ruleset1_name, rule1_name, rule1_add_1, 
        event_type_t::field_write, TestGaia::s_gaia_type, &m_row, "TestGaia.first_name");
}

TEST_F(event_manager_test, log_field_event_multi_event_multi_rule) {
    // Ensure we have field level granularity.
    field_list_t fields;

    // Rule 1 will fire on write to "last_name".
    fields.insert("last_name");
    subscribe_field_rule(TestGaia::s_gaia_type, event_type_t::field_write, fields, m_rule1);

    // Rule 2 will fire on write to "first_name".
    fields.clear();
    fields.insert("first_name");
    subscribe_field_rule(TestGaia::s_gaia_type, event_type_t::field_write, fields, m_rule2);

    int32_t expected_value = m_row.data + rule1_adder;
    EXPECT_EQ(true, log_field_event(&m_row, "last_name", event_type_t::field_write, event_mode_t::immediate));
    validate_rule(expected_value, ruleset1_name, rule1_name, rule1_add_1, 
        event_type_t::field_write, TestGaia::s_gaia_type, &m_row, "TestGaia.last_name");

    expected_value += rule2_adder;
    EXPECT_EQ(true, log_field_event(&m_row, "first_name", event_type_t::field_write, event_mode_t::immediate));        
    validate_rule(expected_value, ruleset1_name, rule2_name, rule2_add_100, 
        event_type_t::field_write, TestGaia::s_gaia_type, &m_row, "TestGaia.first_name");
}

TEST_F(event_manager_test, log_database_event_single_rule_multi_event) 
{
    int32_t expected_value = m_row.data;

    // Bind same rule to update and insert
    subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_update, m_rule1);
    subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_insert, m_rule1);

    // Log delete; verify no rules fired.
    EXPECT_EQ(false, log_database_event(&m_row, event_type_t::row_delete, event_mode_t::immediate));
    validate_rule_not_called();
    
    // Log update followed by insert and verify the rule gets called twice.
    expected_value += rule1_adder;
    EXPECT_EQ(true, log_database_event(&m_row, event_type_t::row_update, event_mode_t::immediate));
    validate_rule(expected_value, ruleset1_name, rule1_name, rule1_add_1, 
        event_type_t::row_update, TestGaia::s_gaia_type, &m_row, m_row.gaia_typename());

    expected_value += rule1_adder;
    EXPECT_EQ(true, log_database_event(&m_row, event_type_t::row_insert, event_mode_t::immediate));
    validate_rule(expected_value, ruleset1_name, rule1_name, rule1_add_1, 
        event_type_t::row_insert, TestGaia::s_gaia_type, &m_row, m_row.gaia_typename());
}

TEST_F(event_manager_test, log_database_event_multi_rule_single_event) 
{
    // Bind two rules to the same event.
    subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_delete, m_rule1);
    subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_delete, m_rule2);

    // Log an update event.  Verify no rules fired.
    EXPECT_EQ(false, log_database_event(&m_row, event_type_t::row_update, event_mode_t::immediate));
    validate_rule_not_called();

    // Verify logging a delete event fires both rules.
    int32_t expected_value = m_row.data + (rule1_adder + rule2_adder);
    EXPECT_EQ(true, log_database_event(&m_row, event_type_t::row_delete, event_mode_t::immediate));
    EXPECT_EQ(m_row.data, expected_value);
}

TEST_F(event_manager_test, log_event_multi_rule_multi_event) 
{
    // See comment on definition of s_rule_decl for which
    // events are setup.
    setup_all_rules();

    // Be super paranoid and ensure we aren't calling the rule function on subscription.
    validate_rule_not_called();

    // Log event for TestGaia::field_write.
    EXPECT_EQ(false, log_field_event(&m_row, "first_name", event_type_t::field_write, event_mode_t::immediate));
    validate_rule_not_called();

    // Log event TestGaia::delete to invoke rule1 and rule2.
    int32_t expected_value = m_row.data + (rule1_adder + rule2_adder);
    EXPECT_EQ(true, log_database_event(&m_row, event_type_t::row_delete, event_mode_t::immediate));
    EXPECT_EQ(m_row.data, expected_value);

    // Unsubscribe rule1 from delete now; log delete; verify only rule2 gets fired.
    expected_value += rule2_adder;
    EXPECT_EQ(true, unsubscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_delete, m_rule1));
    EXPECT_EQ(true, log_database_event(&m_row, event_type_t::row_delete, event_mode_t::immediate));
    validate_rule(expected_value, ruleset1_name, rule2_name, rule2_add_100, 
        event_type_t::row_delete, TestGaia::s_gaia_type, &m_row, m_row.gaia_typename());

    // Insert should invoke rule2.
    expected_value += rule2_adder;
    EXPECT_EQ(true, log_database_event(&m_row, event_type_t::row_insert, event_mode_t::immediate));
    validate_rule(expected_value, ruleset1_name, rule2_name, rule2_add_100, 
        event_type_t::row_insert, TestGaia::s_gaia_type, &m_row, m_row.gaia_typename());

    // Update should invoke rule1.
    expected_value += rule1_adder;
    EXPECT_EQ(true, log_database_event(&m_row, event_type_t::row_update, event_mode_t::immediate));
    validate_rule(expected_value, ruleset1_name, rule1_name, rule1_add_1, 
        event_type_t::row_update, TestGaia::s_gaia_type, &m_row, m_row.gaia_typename());

    // Rollback should invoke rule3, rule4.
    int32_t expected_tx_value = g_tx_data + (rule3_adder + rule4_adder);
    EXPECT_EQ(true, log_database_event(nullptr, event_type_t::transaction_rollback, event_mode_t::immediate));
    EXPECT_EQ(g_tx_data, expected_tx_value);

    // Begin should invoke rule3 only.
    expected_tx_value += rule3_adder;
    EXPECT_EQ(true, log_database_event(nullptr, event_type_t::transaction_begin, event_mode_t::immediate));
    validate_rule(expected_tx_value, ruleset2_name, rule3_name, 
        rule3_add_1000, event_type_t::transaction_begin, 0, nullptr, "");
   
    // commit should invoke rule3, rule4
    expected_tx_value += (rule3_adder + rule4_adder);
    EXPECT_EQ(true, log_database_event(nullptr, event_type_t::transaction_commit, event_mode_t::immediate));
    EXPECT_EQ(g_tx_data, expected_tx_value);
}

TEST_F(event_manager_test, subscribe_database_rule_invalid_event) 
{
    rule_binding_t rb;
    rb.ruleset_name = ruleset1_name;
    rb.rule = rule1_add_1;
    rb.rule_name = rule1_name;

    EXPECT_THROW(subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::field_read, rb), invalid_event_type);
    EXPECT_THROW(subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::field_write, rb), invalid_event_type);
}

TEST_F(event_manager_test, subscribe_field_rule_invalid_event) 
{
    rule_binding_t rb;
    rb.ruleset_name = ruleset2_name;
    rb.rule = rule3_add_1000;
    rb.rule_name = rule3_name;
    field_list_t fields;
    fields.insert("last_name");

    EXPECT_THROW(subscribe_field_rule(0, event_type_t::transaction_begin, fields, rb), invalid_event_type);
    EXPECT_THROW(subscribe_field_rule(0, event_type_t::transaction_commit, fields, rb), invalid_event_type);
    EXPECT_THROW(subscribe_field_rule(0, event_type_t::transaction_rollback, fields, rb), invalid_event_type);
    EXPECT_THROW(subscribe_field_rule(0, event_type_t::row_delete, fields, rb), invalid_event_type);
    EXPECT_THROW(subscribe_field_rule(0, event_type_t::row_insert, fields, rb), invalid_event_type);
    EXPECT_THROW(subscribe_field_rule(0, event_type_t::row_update, fields, rb), invalid_event_type);
}


TEST_F(event_manager_test, unsubscribe_database_rule_invalid_event) 
{
    rule_binding_t rb;
    rb.ruleset_name = ruleset1_name;
    rb.rule = rule1_add_1;
    rb.rule_name = rule1_name;

    EXPECT_THROW(unsubscribe_database_rule(TestGaia::s_gaia_type, event_type_t::field_write, rb), invalid_event_type);
    EXPECT_THROW(unsubscribe_database_rule(TestGaia::s_gaia_type, event_type_t::field_write, rb), invalid_event_type);
}

TEST_F(event_manager_test, subscribe_database_rule_invalid_rule_binding) 
{
    rule_binding_t rb;

    // Empty binding.
    EXPECT_THROW(subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_insert, rb), invalid_rule_binding);

    // No rule_name or rule set.
    rb.ruleset_name = ruleset1_name;
    EXPECT_THROW(subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_update, rb), invalid_rule_binding);

    // No rule set.
    rb.rule_name = rule1_name;
    EXPECT_THROW(subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_update, rb), invalid_rule_binding);
}


TEST_F(event_manager_test, unsubscribe_database_rule_invalid_rule_binding) 
{
    rule_binding_t rb;

    // Empty binding
    EXPECT_THROW(unsubscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_delete, rb), invalid_rule_binding);

    // No rule_name or rule set.
    rb.ruleset_name = ruleset1_name;
    EXPECT_THROW(unsubscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_delete, rb), invalid_rule_binding);

    // No rule set.
    rb.rule_name = rule1_name;
    EXPECT_THROW(unsubscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_delete, rb), invalid_rule_binding);
}

TEST_F(event_manager_test, unsubscribe_database_rule_duplicate_rule) 
{
    rule_binding_t rb;
    rb.ruleset_name = ruleset1_name;
    rb.rule = rule1_add_1;
    rb.rule_name = rule1_name;

    subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_insert, rb);
    EXPECT_THROW(subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_insert, rb), duplicate_rule);

    // Another case of duplicate rule is if we try to bind
    // the same ruleset_name and rule_name to a different rule.
    // Bind to a different event so that it would have been legal
    // if we didn't check for this condition
    rb.rule = rule2_add_100;
    EXPECT_THROW(subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_delete, rb), duplicate_rule);
}

TEST_F(event_manager_test, unsubscribe_database_rule_rule_not_found) 
{
    rule_binding_t rb;
    rb.ruleset_name = ruleset1_name;
    rb.rule_name = rule1_name;
    rb.rule = rule1_add_1;
    
    // Rule not there at all with no rules subscribed.
    EXPECT_EQ(false, unsubscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_update, rb));

    // Subscribe a valid rule.
    subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_update, rb);

    // Try to remove the rule from the other table events that we didn't register the rule on.
    EXPECT_EQ(false, unsubscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_insert, rb));
    EXPECT_EQ(false, unsubscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_delete, rb));

    // Try to remove the rule from a type that we didn't register the rule on
    EXPECT_EQ(false, unsubscribe_database_rule(TestGaia2::s_gaia_type, event_type_t::row_update, rb));

    // With valid rule registered, now ensure that the rule is not found if we change the rule name
    rb.rule_name = rule2_name;
    rb.rule = rule2_add_100;
    EXPECT_EQ(false, unsubscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_update, rb));

    // Ensure we don't find the rule if we change the ruleset_name
    rb.ruleset_name = ruleset2_name;
    rb.rule_name = rule1_name;
    rb.rule = rule1_add_1;
    EXPECT_EQ(false, unsubscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_update, rb));
}

TEST_F(event_manager_test, subscribe_transaction_rule_duplicate_rule) 
{
    rule_binding_t rb;
    rb.ruleset_name = ruleset2_name;
    rb.rule = rule4_add_10000;
    rb.rule_name = "rule4_add_1000";

    subscribe_database_rule(0, event_type_t::transaction_commit, rb);
    EXPECT_THROW(subscribe_database_rule(0, event_type_t::transaction_commit, rb), duplicate_rule);
    
    // Another case of duplicate rule is if we try to bind
    // the same ruleset_name and rule_name to a different rule.
    // Bind to a different event so that it would have been legal
    // if we didn't check for this condition
    rb.rule = rule3_add_1000;
    EXPECT_THROW(subscribe_database_rule(0, event_type_t::transaction_begin, rb), duplicate_rule);
}


TEST_F(event_manager_test, list_rules_none) 
{
    subscription_list_t rules;

    // Verify that the list_subscribed_rules api clears the subscription list the caller
    // passes in.
    rules.push_back(unique_ptr<subscription_t>(new subscription_t({"a", "b", 0, 
        event_type_t::row_update, "first_name"})));

    EXPECT_EQ(1, rules.size());

    list_subscribed_rules(nullptr, nullptr, nullptr, nullptr, rules);
    EXPECT_EQ(0, rules.size());
}

TEST_F(event_manager_test, list_rules_no_filters) 
{
    subscription_list_t rules;
    setup_all_rules();

    list_subscribed_rules(nullptr, nullptr, nullptr, nullptr, rules);
    validate_rule_list(rules, get_expected_subscriptions(nullptr, nullptr, nullptr));
}

TEST_F(event_manager_test, list_rules_ruleset_filter) 
{
    subscription_list_t rules;
    setup_all_rules();

    const char* ruleset_filter = ruleset1_name;
    list_subscribed_rules(ruleset_filter, nullptr, nullptr, nullptr, rules);
    validate_rule_list(rules, get_expected_subscriptions(ruleset_filter, nullptr, nullptr));

    ruleset_filter = ruleset2_name;
    list_subscribed_rules(ruleset_filter, nullptr, nullptr, nullptr, rules);
    validate_rule_list(rules, get_expected_subscriptions(ruleset_filter, nullptr, nullptr));
}

TEST_F(event_manager_test, list_rules_event_type_filter) 
{
    subscription_list_t rules;
    setup_all_rules();

    event_type_t event_filter = event_type_t::transaction_begin;
    list_subscribed_rules(nullptr, nullptr, &event_filter, nullptr, rules);
    validate_rule_list(rules, get_expected_subscriptions(nullptr, nullptr, &event_filter));

    event_filter = event_type_t::row_insert;
    list_subscribed_rules(nullptr, nullptr, &event_filter, nullptr, rules);
    validate_rule_list(rules, get_expected_subscriptions(nullptr, nullptr, &event_filter));
}

TEST_F(event_manager_test, list_rules_gaia_type_filter) 
{
    subscription_list_t rules;
    setup_all_rules();

    gaia_type_t gaia_type_filter = TestGaia2::s_gaia_type;
    list_subscribed_rules(nullptr, &gaia_type_filter, nullptr, nullptr, rules);
    validate_rule_list(rules, get_expected_subscriptions(
        nullptr, &gaia_type_filter, nullptr));

    gaia_type_filter = TestGaia::s_gaia_type;
    list_subscribed_rules(nullptr, &gaia_type_filter, nullptr, nullptr, rules);
    validate_rule_list(rules, get_expected_subscriptions(
        nullptr, &gaia_type_filter, nullptr));
}

TEST_F(event_manager_test, list_rules_all_filters) 
{
    subscription_list_t rules;
    setup_all_rules();

    const char* ruleset_filter = ruleset1_name;
    gaia_type_t gaia_type_filter = TestGaia::s_gaia_type;
    event_type_t event_filter = event_type_t::row_delete;
    list_subscribed_rules(ruleset_filter, &gaia_type_filter, &event_filter, nullptr, rules);
    validate_rule_list(rules, get_expected_subscriptions(ruleset_filter, 
        &gaia_type_filter, &event_filter));

    ruleset_filter = ruleset2_name;
    event_filter = event_type_t::field_write;
    gaia_type_filter = TestGaia2::s_gaia_type;
    list_subscribed_rules(ruleset_filter, &gaia_type_filter, &event_filter, nullptr, rules);
    validate_rule_list(rules, get_expected_subscriptions(ruleset_filter, 
        &gaia_type_filter, &event_filter));
}

TEST_F(event_manager_test, forward_chain_not_subscribed)
{
    subscribe_database_rule(0, event_type_t::transaction_commit, m_rule8);

    int32_t expected_value = g_tx_data + rule8_adder;
    EXPECT_EQ(true, log_database_event(nullptr, event_type_t::transaction_commit, event_mode_t::immediate));
    validate_rule(expected_value, ruleset3_name, rule8_name, 
        rule8_add_100000000, event_type_t::transaction_commit, 0, nullptr, "");
}

TEST_F(event_manager_test, forward_chain_transaction_table)
{
    subscribe_database_rule(0, event_type_t::transaction_commit, m_rule8);
    subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_update, m_rule5);

    int32_t expected_value = g_tx_data + rule8_adder;
    EXPECT_EQ(true, log_database_event(nullptr, event_type_t::transaction_commit, event_mode_t::immediate));
    // Would be nice if we had a global row instead of a member row so that we could refer to it both inside
    // and outside the class for verification
    EXPECT_EQ(g_tx_data, expected_value);
}

TEST_F(event_manager_test, forward_chain_table_transaction)
{
    subscribe_database_rule(TestGaia2::s_gaia_type, event_type_t::row_update, m_rule6);
    subscribe_database_rule(0, event_type_t::transaction_commit, m_rule8);
    
    int32_t expected_table_value = m_row2.data + rule6_adder;
    int32_t expected_transaction_value = g_tx_data + rule8_adder;

    // Because of forward chaining, we expect the table event
    // and the transaction event to be called even though
    // we only logged the table event here.
    EXPECT_EQ(true, log_database_event(&m_row2, event_type_t::row_update, event_mode_t::immediate));
    EXPECT_EQ(m_row2.data, expected_table_value);
    validate_rule(expected_transaction_value, ruleset3_name, rule8_name, 
        rule8_add_100000000, event_type_t::transaction_commit, 0, nullptr, "");
}

TEST_F(event_manager_test, forward_chain_disallow_reentrant)
{
    // See section where rules are defined for the rule heirarchy.
    subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_update, m_rule5);

    int32_t expected_value = m_row.data + rule5_adder;
    EXPECT_EQ(true, log_database_event(&m_row, event_type_t::row_update, event_mode_t::immediate));
    validate_rule(expected_value, 
        ruleset3_name, rule5_name, rule5_add_100000, event_type_t::row_update, TestGaia::s_gaia_type, &m_row, m_row.gaia_typename());
}

TEST_F(event_manager_test, forward_chain_disallow_cycle)
{
    // See section where rules are defined for the rule heirarchy.
    // This test creates a cycle where all the rules are subscribed:
    // rule5 -> rule6 -> rule8 -> rule5.
    subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_update, m_rule5);
    subscribe_database_rule(TestGaia2::s_gaia_type, event_type_t::row_update, m_rule6);
    subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_insert, m_rule7);
    subscribe_database_rule(0, event_type_t::transaction_commit, m_rule8);
    g_forward_chain_cycle = true;

    int32_t expected_table_value = m_row.data + rule5_adder + rule7_adder;
    int32_t expected_transaction_value = g_tx_data + rule8_adder;
    EXPECT_EQ(true, log_database_event(&m_row, event_type_t::row_update, event_mode_t::immediate));
    EXPECT_EQ(expected_transaction_value, g_tx_data);
    validate_rule(expected_table_value, ruleset3_name, rule7_name, 
        rule7_add_10000000, event_type_t::row_insert, TestGaia::s_gaia_type, &m_row, m_row.gaia_typename());
}

TEST_F(event_manager_test, event_logging_no_subscriptions)
{
    clear_event_log();

    // Ensure the event was logged even if it had no subscribers.
    EXPECT_EQ(false, log_field_event(&m_row, "last_name", event_type_t::field_write, event_mode_t::immediate));
    EXPECT_EQ(false, log_database_event(nullptr, event_type_t::transaction_commit, event_mode_t::immediate));
    
    log_entry_t entry(Event_log::get_first());
    verify_event_log_row(*entry, TestGaia::s_gaia_type, 
        event_type_t::field_write, event_mode_t::immediate, "TestGaia.last_name", 0, false); 
    
    entry.reset(entry->get_next());
    verify_event_log_row(*entry, 0, 
        event_type_t::transaction_commit, event_mode_t::immediate, "", 0, false); 

    // Verify we only have two entries in the table.
    entry.reset();
    EXPECT_EQ(2, clear_event_log());
}

TEST_F(event_manager_test, event_logging_subscriptions)
{
    clear_event_log();
    setup_all_rules();

    // Log events with subscriptions and ensure the table is populated.
    EXPECT_EQ(true, log_field_event(&m_row2, "first_name", event_type_t::field_write, event_mode_t::immediate));
    EXPECT_EQ(true, log_database_event(&m_row2, event_type_t::row_insert, event_mode_t::immediate));
    EXPECT_EQ(true, log_database_event(nullptr, event_type_t::transaction_begin, event_mode_t::immediate));

    log_entry_t entry(Event_log::get_first());
    verify_event_log_row(*entry, TestGaia2::s_gaia_type, 
        event_type_t::field_write, event_mode_t::immediate, "TestGaia2.first_name", 0, true); 

    entry.reset(entry->get_next());
    verify_event_log_row(*entry, TestGaia2::s_gaia_type, 
        event_type_t::row_insert, event_mode_t::immediate, "TestGaia2", 0, true); 

    entry.reset(entry->get_next());
    verify_event_log_row(*entry, 0, 
        event_type_t::transaction_begin, event_mode_t::immediate, "", 0, true); 

    entry.reset();
    EXPECT_EQ(3, clear_event_log());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  bool init_engine = true;
  gaia::system::initialize(init_engine);
  return RUN_ALL_TESTS();
}
