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
 * correct context information on invocation.  It also adds the context
 * to a list for verification of what rules were called.  This is particularly
 * useful for the chaining tests.
 */
typedef vector<rule_context_t> rule_context_sequence_t; 
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
        sequence.push_back(*context);
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
    void reset(bool reset_sequence = false) 
    {
        ruleset_name = nullptr;
        rule_name = nullptr;
        rule = nullptr;
        event_type = event_type_t::field_read;
        gaia_type = 0;
        row = nullptr;
        event_source.clear();

        if (reset_sequence)
        {
            sequence.clear();
        }
    }

    void compare_contexts(rule_context_t& a, rule_context_t& b)
    {
        EXPECT_EQ(a.gaia_type, b.gaia_type);
        EXPECT_EQ(a.event_type, b.event_type);
        EXPECT_STREQ(a.event_source.c_str(), b.event_source.c_str());
    }

    // Verifies that rules were called in the order that we expected by
    // comparing the gaia type, event type and source.
    void validate_rule_sequence(rule_context_sequence_t& expected_sequence)
    {
        
        if (sequence.size() == expected_sequence.size())
        {
            for (size_t i = 0; i < sequence.size(); i++)
            {
                compare_contexts(sequence[i], expected_sequence[i]);
            }
        }
        else
        {
            EXPECT_EQ(sequence.size(), expected_sequence.size());
        }
    }

    // Helper to add a context to a context list.
    void add_context_sequence(rule_context_sequence_t& sequence, gaia_type_t gaia_type, event_type_t event_type, const char* event_source)
    {
        rule_binding_t ignore;
        rule_context_t c(ignore, gaia_type, event_type, nullptr, nullptr);
        c.event_source = event_source ? event_source : "";
        sequence.push_back(c);
    }

    gaia_type_t gaia_type;
    gaia_base_t* row;
    const char* ruleset_name;
    const char* rule_name;
    gaia_rule_fn rule;
    event_type_t event_type;
    string event_source;

    rule_context_sequence_t sequence;
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
    {
    }

    static const gaia_type_t s_gaia_type;
    gaia_type_t gaia_type() override
    {
        return s_gaia_type;
    }

    void reset(bool) override {}
};
const gaia_type_t TestGaia::s_gaia_type = 333;

// Only to test gaia type filters on the
// list_subscribed_rules api.
class TestGaia2 : public gaia_base_t
{
public:
    TestGaia2() 
    : gaia_base_t("TestGaia2")
    {
    }

    static const gaia_type_t s_gaia_type;
    gaia_type_t gaia_type() override
    {
        return s_gaia_type;
    }

    void reset(bool) override {}
};
const gaia_type_t TestGaia2::s_gaia_type = 444;


 /**
 * Following constructs are used to verify the list_subscribed_rules API
 * returns the correct rules based on the filter criteria to the API.  It
 * also is used to do table-driven tests on different rule binding configurations.
 */ 
typedef std::unordered_map<string, subscription_t> map_subscriptions_t;
static constexpr char ruleset1_name[] = "Ruleset_1";
static constexpr char ruleset2_name[] = "Ruleset_2";
static constexpr char ruleset3_name[] = "Ruleset_3";
static constexpr char rule1_name[] = "rule1";
static constexpr char rule2_name[] = "rule2";
static constexpr char rule3_name[] = "rule3";
static constexpr char rule4_name[] = "rule4";
static constexpr char rule5_name[] = "rule5";
static constexpr char rule6_name[] = "rule6";
static constexpr char rule7_name[] = "rule7";
static constexpr char rule8_name[] = "rule8";
static constexpr char rule9_name[] = "rule9";
static constexpr char rule10_name[] = "rule10";

/**
 * Table Rule functions.
 * 
 * Make sure the rule operations are commutative since we don't guarantee
 * the order that rules are fired.  Table rules write to the passed in "row"
 * data.
 */
void rule1(const rule_context_t* context)
{
    g_context_checker.set(context);
}

void rule2(const rule_context_t* context)
{
    g_context_checker.set(context);
}

/**
 * Transaction Rule functions
 * 
 * Make sure the rule operations are commutative since we don't guarantee
 * the order that rules are fired.  Transaction rules don't have any
 * associated context so they write to a global variable.
 */
void rule3(const rule_context_t* context)
{
    g_context_checker.set(context);
}

void rule4(const rule_context_t* context)
{
    g_context_checker.set(context);
}

/**
 * Rule 5 handles an TestGaia::update event
 * Attempts to forward chain to TestGaia::update (disallowed: same gaia_type and event_type)
 * [Rule 6] Forward chains to TestGaia2::update (allowed: different gaia_type)
 * [Rule 7] Forward chains to TestGaia::insert (allowed: different event_type)
 */
void rule5(const rule_context_t* context)
{
    TestGaia * row = static_cast<TestGaia *>(context->event_context);
    g_context_checker.set(context);

    // Disallow reentrant event call.
    log_database_event(row, context->event_type, event_mode_t::immediate);

    // Allow event call on different gaia_type.
    TestGaia2 obj2;
    log_database_event(&obj2, context->event_type, event_mode_t::immediate);

    // Allow event call on different event_type.
    log_database_event(row, event_type_t::row_insert, event_mode_t::immediate);
}

/**
 * Rule 6 handles TestGaia2::update
 * [Rule 8] Forward chains to Transaction::commit event (allowed: different event class)
 */ 
void rule6(const rule_context_t* context)
{
    g_context_checker.set(context);

    // Allow different event class (transaction event, not table event)
    log_database_event(nullptr, event_type_t::transaction_commit, event_mode_t::immediate);
}

/**
 * Rule 7 handles TestGaia::insert
 */
void rule7(const rule_context_t* context)
{
    g_context_checker.set(context);
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
void rule8(const rule_context_t* context)
{
    g_context_checker.set(context);

    TestGaia row;

    // We expect the rule to be fired only if it is subscribed
    // and we are not in a forward chain cycle.
    log_database_event(&row, event_type_t::row_update, event_mode_t::immediate);
}

/**
 * Rule 9 handles (TestGaia.timestamp read) and (TestGaia.value write).
 * Attempts to forward chain to 
 * [Rule 9] TestGaia.timestamp read
 * [Rule 9] TestGaia.value write
 * [Rule 10] TestGaia.id read
 */
void rule9(const rule_context_t* context)
{
    g_context_checker.set(context);

    TestGaia * row = static_cast<TestGaia *>(context->event_context);

    // This is legal if we were fired because of "value write"
    log_field_event(row, "timestamp", event_type_t::field_read, event_mode_t::immediate);
    // Reading a value is not bound to a rule so this call is fine, although no rule is executed
    log_field_event(row, "value", event_type_t::field_read, event_mode_t::immediate);
    // This is legal if we were fired because of "timestamp read".
    log_field_event(row, "value", event_type_t::field_write, event_mode_t::immediate);
    // Call rule 10
    log_field_event(row, "id", event_type_t::field_read, event_mode_t::immediate);
}

/**
 * Rule 10 handles (TestGaia.id read)
 * Attempts to forward chain to 
 * [Rule 9] TestGaia.value write
 */
void rule10(const rule_context_t* context)
{
    g_context_checker.set(context);
    TestGaia * row = static_cast<TestGaia *>(context->event_context);

    log_field_event(row, "value", event_type_t::field_write, event_mode_t::immediate);
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
    {{ruleset1_name, rule1_name, TestGaia::s_gaia_type, event_type_t::row_delete, nullptr}, rule1},
    {{ruleset1_name, rule2_name, TestGaia::s_gaia_type, event_type_t::row_delete, nullptr}, rule2},
    {{ruleset1_name, rule2_name, TestGaia::s_gaia_type, event_type_t::row_insert, nullptr}, rule2},
    {{ruleset1_name, rule1_name, TestGaia::s_gaia_type, event_type_t::row_update, nullptr}, rule1},
    {{ruleset2_name, rule3_name, TestGaia2::s_gaia_type, event_type_t::row_insert, nullptr}, rule3},
    {{ruleset2_name, rule4_name, TestGaia2::s_gaia_type, event_type_t::row_insert, nullptr}, rule4},
    {{ruleset1_name, rule1_name, TestGaia2::s_gaia_type, event_type_t::field_write, "first_name"}, rule1},
    {{ruleset1_name, rule2_name, TestGaia2::s_gaia_type, event_type_t::field_write, "last_name"}, rule2},
    {{ruleset2_name, rule3_name, 0, event_type_t::transaction_begin, nullptr}, rule3},
    {{ruleset2_name, rule3_name, 0, event_type_t::transaction_commit, nullptr}, rule3},
    {{ruleset2_name, rule4_name, 0, event_type_t::transaction_commit, nullptr}, rule4},
    {{ruleset2_name, rule3_name, 0, event_type_t::transaction_rollback, nullptr}, rule3},
    {{ruleset2_name, rule4_name, 0, event_type_t::transaction_rollback, nullptr}, rule4}
};
static constexpr int s_rule_decl_len = sizeof(s_rule_decl)/sizeof(s_rule_decl[0]);

 /**
 * Applications must provide an implementation for initialize_rules().
 * This function is called on initialization of the singleton event
 * manager instance and is used to regisgter rules with the system.  
 * If this function is not called then every test will fail below 
 * because the condition is checked on TearDown() of
 * ever test case in the test fixture.  In addition, verify that we
 * actually can subscribe a rule in this intialize_rules function.  
 */
 uint32_t g_initialize_rules_called = 0;

 extern "C"
 void initialize_rules()
 {
     ++g_initialize_rules_called;
     rule_binding_t binding("test", "test", rule1);
     subscribe_database_rule(TestGaia2::s_gaia_type, event_type_t::row_delete, binding);
     unsubscribe_rules();
 }

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
    }

    virtual void TearDown()
    {
        gaia_base_t::commit_transaction();
        unsubscribe_rules();
        g_context_checker.reset(true);

        // This expectation verifies that the caller provided
        // initialize_rules function was called exactly once by
        // the event_manager_t singleton.
        EXPECT_EQ(1, g_initialize_rules_called);
    }

    void validate_rule(
        const char* ruleset_name,
        const char* rule_name,
        gaia_rule_fn rule,
        event_type_t type,
        gaia_type_t gaia_type,
        gaia_base_t* row,
        const char* source) 

    {
        g_context_checker.validate(ruleset_name, rule_name, rule, type, gaia_type, row, source);
    }

    void add_context_sequence(rule_context_sequence_t& sequence, gaia_type_t gaia_type, event_type_t event_type, const char* event_source)
    {
        g_context_checker.add_context_sequence(sequence, gaia_type, event_type, event_source);
    }

    void validate_rule_sequence(rule_context_sequence_t& expected)
    {
        g_context_checker.validate_rule_sequence(expected);
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
    rule_binding_t m_rule1{ruleset1_name, rule1_name, rule1};
    rule_binding_t m_rule2{ruleset1_name, rule2_name, rule2};
    rule_binding_t m_rule3{ruleset2_name, rule3_name, rule3};
    rule_binding_t m_rule4{ruleset2_name, rule4_name, rule4};
    rule_binding_t m_rule5{ruleset3_name, rule5_name, rule5};
    rule_binding_t m_rule6{ruleset3_name, rule6_name, rule6};
    rule_binding_t m_rule7{ruleset3_name, rule7_name, rule7};
    rule_binding_t m_rule8{ruleset3_name, rule8_name, rule8};
    rule_binding_t m_rule9{ruleset3_name, rule9_name, rule9};
    rule_binding_t m_rule10{ruleset3_name, rule10_name, rule10};
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
    // An empty sequence will verify that the rule was not called.
    rule_context_sequence_t sequence;

    EXPECT_EQ(false, log_database_event(&m_row, event_type_t::row_delete, event_mode_t::immediate));
    validate_rule_sequence(sequence);
}

TEST_F(event_manager_test, log_database_event_single_event_single_rule) {
    // Subscribe to update.
    subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_update, m_rule1);

    // An empty sequence will verify that the rule was not called.
    rule_context_sequence_t sequence;

    // Log insert; verify the rule was not fired because it is bound to update, not insert.
    EXPECT_EQ(false, log_database_event(&m_row, event_type_t::row_insert, event_mode_t::immediate));
    validate_rule_sequence(sequence);

    // Now we will fire the update event.
    add_context_sequence(sequence, m_row.gaia_type(), event_type_t::row_update, m_row.gaia_typename());    

    EXPECT_EQ(true, log_database_event(&m_row, event_type_t::row_update, event_mode_t::immediate));
    validate_rule_sequence(sequence);
    validate_rule(ruleset1_name, rule1_name, rule1, 
        event_type_t::row_update, TestGaia::s_gaia_type, &m_row, m_row.gaia_typename());
}


TEST_F(event_manager_test, log_field_event_single_event_single_rule) {
    // Ensure we have field level granularity.
    field_list_t fields;

    // Binding to an empty field list won't fire any rules.
    subscribe_field_rule(TestGaia::s_gaia_type, event_type_t::field_write, fields, m_rule1);

    // An empty sequence will verify that the rule was not called.
    rule_context_sequence_t sequence;

    EXPECT_EQ(false, log_field_event(&m_row, "last_name", event_type_t::field_write, event_mode_t::immediate));
    validate_rule_sequence(sequence);

    // Verify that no rules were subscribed for an emtpy field list.
    EXPECT_EQ(false, unsubscribe_field_rule(TestGaia::s_gaia_type, event_type_t::field_write, fields, m_rule1));

    fields.insert("last_name");
    subscribe_field_rule(TestGaia::s_gaia_type, event_type_t::field_write, fields, m_rule1);

    add_context_sequence(sequence, m_row.gaia_type(), event_type_t::field_write, "TestGaia.last_name");
    
    EXPECT_EQ(true, log_field_event(&m_row, "last_name", event_type_t::field_write, event_mode_t::immediate));
    validate_rule_sequence(sequence);
    validate_rule(ruleset1_name, rule1_name, rule1, event_type_t::field_write, 
        TestGaia::s_gaia_type, &m_row, "TestGaia.last_name");
}

TEST_F(event_manager_test, log_field_event_multi_event_single_rule) {
    // Ensure we have field level granularity.
    field_list_t fields;

    // Rule 1 will fire on any writes to "last_name" or "first_name"
    fields.insert("last_name");
    fields.insert("first_name");
    subscribe_field_rule(TestGaia::s_gaia_type, event_type_t::field_write, fields, m_rule1);

    rule_context_sequence_t sequence;
    add_context_sequence(sequence, m_row.gaia_type(), event_type_t::field_write, "TestGaia.last_name");
    EXPECT_EQ(true, log_field_event(&m_row, "last_name", event_type_t::field_write, event_mode_t::immediate));
    validate_rule_sequence(sequence);
    validate_rule(ruleset1_name, rule1_name, rule1, 
        event_type_t::field_write, TestGaia::s_gaia_type, &m_row, "TestGaia.last_name");

    add_context_sequence(sequence, m_row.gaia_type(), event_type_t::field_write, "TestGaia.first_name");
    EXPECT_EQ(true, log_field_event(&m_row, "first_name", event_type_t::field_write, event_mode_t::immediate));
    validate_rule_sequence(sequence);
    validate_rule(ruleset1_name, rule1_name, rule1, event_type_t::field_write, 
        TestGaia::s_gaia_type, &m_row, "TestGaia.first_name");
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

    rule_context_sequence_t sequence;
    add_context_sequence(sequence, m_row.gaia_type(), event_type_t::field_write, "TestGaia.last_name");

    EXPECT_EQ(true, log_field_event(&m_row, "last_name", event_type_t::field_write, event_mode_t::immediate));
    validate_rule_sequence(sequence);
    validate_rule(ruleset1_name, rule1_name, rule1, event_type_t::field_write, 
        TestGaia::s_gaia_type, &m_row, "TestGaia.last_name");

    add_context_sequence(sequence, m_row.gaia_type(), event_type_t::field_write, "TestGaia.first_name");
    EXPECT_EQ(true, log_field_event(&m_row, "first_name", event_type_t::field_write, event_mode_t::immediate));
    validate_rule_sequence(sequence);
    validate_rule(ruleset1_name, rule2_name, rule2, event_type_t::field_write, 
        TestGaia::s_gaia_type, &m_row, "TestGaia.first_name");
}

TEST_F(event_manager_test, log_database_event_single_rule_multi_event) 
{
    // Bind same rule to update and insert
    subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_update, m_rule1);
    subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_insert, m_rule1);

    rule_context_sequence_t sequence;

    // Log delete; verify no rules fired.
    EXPECT_EQ(false, log_database_event(&m_row, event_type_t::row_delete, event_mode_t::immediate));
    validate_rule_sequence(sequence);

    add_context_sequence(sequence, m_row.gaia_type(), event_type_t::row_update, m_row.gaia_typename());
    add_context_sequence(sequence, m_row.gaia_type(), event_type_t::row_insert, m_row.gaia_typename());
    
    // Log update followed by insert and verify the rule gets called twice.
    EXPECT_EQ(true, log_database_event(&m_row, event_type_t::row_update, event_mode_t::immediate));
    EXPECT_EQ(true, log_database_event(&m_row, event_type_t::row_insert, event_mode_t::immediate));
    validate_rule_sequence(sequence);
}

TEST_F(event_manager_test, log_database_event_multi_rule_single_event) 
{
    // Bind two rules to the same event.
    subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_delete, m_rule1);
    subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_delete, m_rule2);

    rule_context_sequence_t sequence;

    // Log an update event.  Verify no rules fired.
    EXPECT_EQ(false, log_database_event(&m_row, event_type_t::row_update, event_mode_t::immediate));
    validate_rule_sequence(sequence);

    // Verify logging a delete event fires both rules.
    add_context_sequence(sequence, m_row.gaia_type(), event_type_t::row_delete, m_row.gaia_typename());
    add_context_sequence(sequence, m_row.gaia_type(), event_type_t::row_delete, m_row.gaia_typename());

    EXPECT_EQ(true, log_database_event(&m_row, event_type_t::row_delete, event_mode_t::immediate));
    validate_rule_sequence(sequence);
}

TEST_F(event_manager_test, log_event_multi_rule_multi_event) 
{
    // See comment on definition of s_rule_decl for which
    // events are setup.
    setup_all_rules();

    // Be super paranoid and ensure we aren't calling the rule function on subscription.
    rule_context_sequence_t sequence;
    validate_rule_sequence(sequence);

    // Log event for TestGaia::field_write.
    EXPECT_EQ(false, log_field_event(&m_row, "first_name", event_type_t::field_write, event_mode_t::immediate));
    validate_rule_sequence(sequence);

    // Log event TestGaia::delete to invoke rule1 and rule2.
    add_context_sequence(sequence, m_row.gaia_type(), event_type_t::row_delete, m_row.gaia_typename());
    add_context_sequence(sequence, m_row.gaia_type(), event_type_t::row_delete, m_row.gaia_typename());
    EXPECT_EQ(true, log_database_event(&m_row, event_type_t::row_delete, event_mode_t::immediate));
    validate_rule_sequence(sequence);
    
    // Unsubscribe rule1 from delete now; log delete; verify only rule2 gets fired.
    EXPECT_EQ(true, unsubscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_delete, m_rule1));
    add_context_sequence(sequence, m_row.gaia_type(), event_type_t::row_delete, m_row.gaia_typename());
    EXPECT_EQ(true, log_database_event(&m_row, event_type_t::row_delete, event_mode_t::immediate));
    validate_rule_sequence(sequence);
    validate_rule(ruleset1_name, rule2_name, rule2, event_type_t::row_delete, TestGaia::s_gaia_type, &m_row, m_row.gaia_typename());

    // Insert should invoke rule2.
    add_context_sequence(sequence, m_row.gaia_type(), event_type_t::row_insert, m_row.gaia_typename());
    EXPECT_EQ(true, log_database_event(&m_row, event_type_t::row_insert, event_mode_t::immediate));
    validate_rule_sequence(sequence);
    validate_rule(ruleset1_name, rule2_name, rule2, event_type_t::row_insert, 
        TestGaia::s_gaia_type, &m_row, m_row.gaia_typename());

    // Update should invoke rule1.
    add_context_sequence(sequence, m_row.gaia_type(), event_type_t::row_update, m_row.gaia_typename());
    EXPECT_EQ(true, log_database_event(&m_row, event_type_t::row_update, event_mode_t::immediate));
    validate_rule_sequence(sequence);
    validate_rule(ruleset1_name, rule1_name, rule1, event_type_t::row_update, 
        TestGaia::s_gaia_type, &m_row, m_row.gaia_typename());

    // Rollback should invoke rule3, rule4.
    add_context_sequence(sequence, 0, event_type_t::transaction_rollback, nullptr);
    add_context_sequence(sequence, 0, event_type_t::transaction_rollback, nullptr);
    EXPECT_EQ(true, log_database_event(nullptr, event_type_t::transaction_rollback, event_mode_t::immediate));
    validate_rule_sequence(sequence);

    // Begin should invoke rule3 only.
    add_context_sequence(sequence, 0, event_type_t::transaction_begin, nullptr);
    EXPECT_EQ(true, log_database_event(nullptr, event_type_t::transaction_begin, event_mode_t::immediate));
    validate_rule_sequence(sequence);
    validate_rule(ruleset2_name, rule3_name, rule3, event_type_t::transaction_begin, 0, nullptr, "");
   
    // commit should invoke rule3, rule4
    add_context_sequence(sequence, 0, event_type_t::transaction_commit, nullptr);
    add_context_sequence(sequence, 0, event_type_t::transaction_commit, nullptr);
    EXPECT_EQ(true, log_database_event(nullptr, event_type_t::transaction_commit, event_mode_t::immediate));
    validate_rule_sequence(sequence);
}

TEST_F(event_manager_test, subscribe_database_rule_invalid_event) 
{
    rule_binding_t rb;
    rb.ruleset_name = ruleset1_name;
    rb.rule = rule1;
    rb.rule_name = rule1_name;

    EXPECT_THROW(subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::field_read, rb), invalid_event_type);
    EXPECT_THROW(subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::field_write, rb), invalid_event_type);
}

TEST_F(event_manager_test, subscribe_field_rule_invalid_event) 
{
    rule_binding_t rb;
    rb.ruleset_name = ruleset2_name;
    rb.rule = rule3;
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
    rb.rule = rule1;
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
    rb.rule = rule1;
    rb.rule_name = rule1_name;

    subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_insert, rb);
    EXPECT_THROW(subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_insert, rb), duplicate_rule);

    // Another case of duplicate rule is if we try to bind
    // the same ruleset_name and rule_name to a different rule.
    // Bind to a different event so that it would have been legal
    // if we didn't check for this condition
    rb.rule = rule2;
    EXPECT_THROW(subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_delete, rb), duplicate_rule);
}

TEST_F(event_manager_test, unsubscribe_database_rule_rule_not_found) 
{
    rule_binding_t rb;
    rb.ruleset_name = ruleset1_name;
    rb.rule_name = rule1_name;
    rb.rule = rule1;
    
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
    rb.rule = rule2;
    EXPECT_EQ(false, unsubscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_update, rb));

    // Ensure we don't find the rule if we change the ruleset_name
    rb.ruleset_name = ruleset2_name;
    rb.rule_name = rule1_name;
    rb.rule = rule1;
    EXPECT_EQ(false, unsubscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_update, rb));
}

TEST_F(event_manager_test, subscribe_transaction_rule_duplicate_rule) 
{
    rule_binding_t rb;
    rb.ruleset_name = ruleset2_name;
    rb.rule = rule4;
    rb.rule_name = "rule4";

    subscribe_database_rule(0, event_type_t::transaction_commit, rb);
    EXPECT_THROW(subscribe_database_rule(0, event_type_t::transaction_commit, rb), duplicate_rule);
    
    // Another case of duplicate rule is if we try to bind
    // the same ruleset_name and rule_name to a different rule.
    // Bind to a different event so that it would have been legal
    // if we didn't check for this condition
    rb.rule = rule3;
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
    
    rule_context_sequence_t expected;
    add_context_sequence(expected, 0, event_type_t::transaction_commit, nullptr);

    EXPECT_EQ(true, log_database_event(nullptr, event_type_t::transaction_commit, event_mode_t::immediate));
    validate_rule_sequence(expected);
}

TEST_F(event_manager_test, forward_chain_transaction_table)
{
    subscribe_database_rule(0, event_type_t::transaction_commit, m_rule8);
    subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_update, m_rule5);

    rule_context_sequence_t expected;
    add_context_sequence(expected, 0, event_type_t::transaction_commit, nullptr);
    add_context_sequence(expected, m_row.gaia_type(), event_type_t::row_update, m_row.gaia_typename());

    EXPECT_EQ(true, log_database_event(nullptr, event_type_t::transaction_commit, event_mode_t::immediate));
    validate_rule_sequence(expected);
}

TEST_F(event_manager_test, forward_chain_table_transaction)
{
    subscribe_database_rule(TestGaia2::s_gaia_type, event_type_t::row_update, m_rule6);
    subscribe_database_rule(0, event_type_t::transaction_commit, m_rule8);
    
    // Because of forward chaining, we expect the table event
    // and the transaction event to be called even though
    // we only logged the table event here.
    rule_context_sequence_t expected;
    add_context_sequence(expected, m_row2.gaia_type(), event_type_t::row_update, m_row2.gaia_typename());
    add_context_sequence(expected, 0, event_type_t::transaction_commit, nullptr);

    EXPECT_EQ(true, log_database_event(&m_row2, event_type_t::row_update, event_mode_t::immediate));
    validate_rule_sequence(expected);
}

TEST_F(event_manager_test, forward_chain_disallow_reentrant)
{
    // See section where rules are defined for the rule heirarchy.
    subscribe_database_rule(TestGaia::s_gaia_type, event_type_t::row_update, m_rule5);

    rule_context_sequence_t expected;
    add_context_sequence(expected, m_row.gaia_type(), event_type_t::row_update, m_row.gaia_typename());

    EXPECT_EQ(true, log_database_event(&m_row, event_type_t::row_update, event_mode_t::immediate));
    validate_rule_sequence(expected);
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

    rule_context_sequence_t expected;
    add_context_sequence(expected, m_row.gaia_type(), event_type_t::row_update, m_row.gaia_typename());
    add_context_sequence(expected, m_row2.gaia_type(), event_type_t::row_update, m_row2.gaia_typename());
    add_context_sequence(expected, 0, event_type_t::transaction_commit, nullptr);
    add_context_sequence(expected, m_row.gaia_type(), event_type_t::row_insert, m_row.gaia_typename());

    EXPECT_EQ(true, log_database_event(&m_row, event_type_t::row_update, event_mode_t::immediate));
    validate_rule_sequence(expected);
}

TEST_F(event_manager_test, forward_chain_field_not_subscribed)
{
    field_list_t fields;
    fields.insert("timestamp");
    subscribe_field_rule(TestGaia::s_gaia_type, event_type_t::field_read, fields, m_rule9);

    // expect the following calls
    rule_context_sequence_t expected;
    add_context_sequence(expected, m_row.gaia_type(), event_type_t::field_read, "TestGaia.timestamp");

    EXPECT_EQ(true, log_field_event(&m_row, "timestamp", event_type_t::field_read, event_mode_t::immediate));
    validate_rule_sequence(expected);
}

TEST_F(event_manager_test, forward_chain_field_self_subscribed)
{
    field_list_t fields;
    fields.insert("timestamp");
    subscribe_field_rule(TestGaia::s_gaia_type, event_type_t::field_read, fields, m_rule9);
    fields.clear();

    fields.insert("value");
    subscribe_field_rule(TestGaia::s_gaia_type, event_type_t::field_write, fields, m_rule9);

    // Expect the following sequence of calls.
    rule_context_sequence_t expected;
    add_context_sequence(expected, m_row.gaia_type(), event_type_t::field_read, "TestGaia.timestamp");
    add_context_sequence(expected, m_row.gaia_type(), event_type_t::field_write, "TestGaia.value");

    EXPECT_EQ(true, log_field_event(&m_row, "timestamp", event_type_t::field_read, event_mode_t::immediate));
    validate_rule_sequence(expected);

    add_context_sequence(expected, m_row.gaia_type(), event_type_t::field_write, "TestGaia.value");
    add_context_sequence(expected, m_row.gaia_type(), event_type_t::field_read, "TestGaia.timestamp");
    
    EXPECT_EQ(true, log_field_event(&m_row, "value", event_type_t::field_write, event_mode_t::immediate));
    validate_rule_sequence(expected);        
}

TEST_F(event_manager_test, forward_chain_field_multi_subscribed)
{
    field_list_t fields;
    fields.insert("timestamp");
    subscribe_field_rule(TestGaia::s_gaia_type, event_type_t::field_read, fields, m_rule9);
    fields.clear();

    fields.insert("value");
    subscribe_field_rule(TestGaia::s_gaia_type, event_type_t::field_write, fields, m_rule9);
    fields.clear();

    fields.insert("id");
    subscribe_field_rule(TestGaia::s_gaia_type, event_type_t::field_read, fields, m_rule10);
    fields.clear();

    // Expect the following sequence of calls.  Note that the same rule handles both timestamp read
    // and value write so we actually do re-enter the rule which leads to this call chain.
    rule_context_sequence_t expected;
    add_context_sequence(expected, m_row.gaia_type(), event_type_t::field_read, "TestGaia.timestamp");
    add_context_sequence(expected, m_row.gaia_type(), event_type_t::field_write, "TestGaia.value");
    add_context_sequence(expected, m_row.gaia_type(), event_type_t::field_read, "TestGaia.id");
    add_context_sequence(expected, m_row.gaia_type(), event_type_t::field_read, "TestGaia.id");
    add_context_sequence(expected, m_row.gaia_type(), event_type_t::field_write, "TestGaia.value");


    EXPECT_EQ(true, log_field_event(&m_row, "timestamp", event_type_t::field_read, event_mode_t::immediate));
    validate_rule_sequence(expected);

    add_context_sequence(expected, m_row.gaia_type(), event_type_t::field_read, "TestGaia.id");
    add_context_sequence(expected, m_row.gaia_type(), event_type_t::field_write, "TestGaia.value");
    add_context_sequence(expected, m_row.gaia_type(), event_type_t::field_read, "TestGaia.timestamp");

    EXPECT_EQ(true, log_field_event(&m_row, "id", event_type_t::field_read, event_mode_t::immediate));
    validate_rule_sequence(expected);
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
