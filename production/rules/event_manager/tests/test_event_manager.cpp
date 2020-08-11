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
#include "gaia_event_log.h"
#include "triggers.hpp"
#include "db_test_base.hpp"
#include "event_manager_test_helpers.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::direct_access;
using namespace gaia::rules;
using namespace std;
using namespace gaia::db::triggers;

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
        event_type = context->event_type;
        gaia_type = context->gaia_type;
        record = context->record;
        sequence.push_back(*context);
    }

    void validate(
        event_type_t a_event_type,
        gaia_type_t a_gaia_type,
        gaia_id_t a_record)
    {
        EXPECT_EQ(event_type, a_event_type);
        EXPECT_EQ(gaia_type, a_gaia_type);
        EXPECT_EQ(record, a_record);
        reset();
    }

    void reset(bool reset_sequence = false)
    {
        event_type = event_type_t::row_delete;
        gaia_type = 0;
        record = 0;

        if (reset_sequence)
        {
            sequence.clear();
        }
    }

    void compare_contexts(rule_context_t& a, rule_context_t& b)
    {
        EXPECT_EQ(a.gaia_type, b.gaia_type);
        EXPECT_EQ(a.event_type, b.event_type);
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
    void add_context_sequence(rule_context_sequence_t& sequence, gaia_type_t gaia_type, event_type_t event_type)
    {
        rule_context_t c(get_dummy_transaction(), gaia_type, event_type, 0);
        sequence.push_back(c);
    }

    auto_transaction_t& get_dummy_transaction(bool init=false) 
    {
        // Create a transaction that won't do anything
        static auto_transaction_t s_dummy(auto_transaction_t::no_auto_begin);
        if (init) {
            s_dummy.commit();
        }
        return s_dummy;
    }

    gaia_type_t gaia_type;
    gaia_id_t record;
    const char* ruleset_name;
    const char* rule_name;
    gaia_rule_fn rule;
    event_type_t event_type;
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
    : TestGaia(0)
    {
    }

    TestGaia(gaia_id_t record)
    : gaia_base_t("TestGaia")
    , m_id(record)
    {

    }

    static const gaia_type_t s_gaia_type;
    gaia_type_t gaia_type() override
    {
        return s_gaia_type;
    }

    gaia_id_t gaia_id() { return m_id; }

    gaia_id_t m_id;
};
const gaia_type_t TestGaia::s_gaia_type = 333;
typedef unique_ptr<TestGaia> test_gaia_ptr_t;

// Only to test gaia type filters on the
// list_subscribed_rules api.
class TestGaia2 : public gaia_base_t
{
public:
    TestGaia2()
    : TestGaia2(0)
    {
    }

    TestGaia2(gaia_id_t record)
    : gaia_base_t("TestGaia2")
    , m_id(record)
    {
    }

    static const gaia_type_t s_gaia_type;
    gaia_type_t gaia_type() override
    {
        return s_gaia_type;
    }

    gaia_id_t gaia_id() { return m_id; }
    gaia_id_t m_id;
};

const gaia_type_t TestGaia2::s_gaia_type = 444;
typedef unique_ptr<TestGaia2> test_gaia2_ptr_t;


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
//static constexpr char rule6_name[] = "rule6";
static constexpr char rule7_name[] = "rule7";
static constexpr char rule8_name[] = "rule8";
static constexpr char rule9_name[] = "rule9";
static constexpr char rule10_name[] = "rule10";

// field ordinals used throughout the test
static constexpr uint16_t s_first_name = 3;
static constexpr uint16_t s_last_name = 4;
static constexpr uint16_t s_value = 5;
static constexpr uint16_t s_timestamp = 6;
static constexpr uint16_t s_id = 7;

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
    test_gaia_ptr_t test_gaia_ptr(new TestGaia(context->record));
    g_context_checker.set(context);

    TestGaia2 obj2(42);
    trigger_event_t trigger_events[] = {
        // Allow same event, we should not be re-entrant cause rules are only fired
        // after the rule invocation returns
        // TODO[GAIAPLAT-155]: Determine how the rule schedule deals with cycles
        //{context->event_type, context->gaia_type, context->record, nullptr, 0},
        // Allow event call on different gaia_type.
        {context->event_type, TestGaia2::s_gaia_type, context->record, nullptr, 0},
        // Allow event call on different event_type.
        {event_type_t::row_insert, context->gaia_type, test_gaia_ptr->gaia_id(), nullptr, 0}
    };
    test::commit_trigger(0, trigger_events, 2);
}

/**
 * Rule 6 handles TestGaia2::update
 * [Rule 8] Forward chains to Transaction::commit event (allowed: different event class)
 * 
 * TODO[GAIAPLAT-194]: Transaction events are out of scope for Q2
void rule6(const rule_context_t* context)
{
    g_context_checker.set(context);

    // Allow different event class (transaction event, not table event)
    trigger_event_t trigger_event = {event_type_t::transaction_commit, 0, 0, nullptr, 0};
    test::commit_trigger(0, &trigger_event, 1);
}
*/

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
    trigger_event_t trigger_event = {event_type_t::row_update, TestGaia::s_gaia_type, row.gaia_id(), nullptr, 0};
    test::commit_trigger(0, &trigger_event, 1);
}

/**
 * Rule 9 handles (TestGaia.value write).
 *
 * Attempts to forward chain to
 * [Rule 10] TestGaia.timestamp write
 *
 * TODO[GAIAPLAT-155]: forward chaining to self should be disallowed
 */
void rule9(const rule_context_t* context)
{
    g_context_checker.set(context);

    // write to timestamp value an id values, writing the timestamp
    // will chain to the rule10
    trigger_event_t events[] = {
        {event_type_t::row_update, TestGaia::s_gaia_type, context->record, &s_timestamp, 1},
        {event_type_t::row_update, TestGaia::s_gaia_type, context->record, &s_id, 1},
    };
    test::commit_trigger(0, events, 2);
}

/**
 * Rule 10 handles (TestGaia.timestamp write)
 * Attempts to forward chain to
 * [Rule 9] TestGaia.value write
 */
void rule10(const rule_context_t* context)
{
    g_context_checker.set(context);

    //TODO[GAIAPLAT-155]
    //trigger_event_t event = {event_type_t::row_update, TestGaia::s_gaia_type, context->record, &s_value, 1};
    //commit_trigger(0, &event, 1);
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
 * field_write [first_name(3), last_name(4)]: rule1, rule2
 * row_insert: rule3, rule4
 *
 * TODO[GAIAPLAT-194]: Transaction Events are out of scope for Q2
 * transaction_begin: rule3
 * transaction_commit: rule3, rule4
 * transaction_rollback: rule3, rule4
 */
static constexpr rule_decl_t s_rule_decl[] = {
    {{ruleset1_name, rule1_name, TestGaia::s_gaia_type, event_type_t::row_delete, 0}, rule1},
    {{ruleset1_name, rule2_name, TestGaia::s_gaia_type, event_type_t::row_delete, 0}, rule2},
    {{ruleset1_name, rule2_name, TestGaia::s_gaia_type, event_type_t::row_insert, 0}, rule2},
    {{ruleset1_name, rule1_name, TestGaia::s_gaia_type, event_type_t::row_update, 0}, rule1},
    {{ruleset2_name, rule3_name, TestGaia2::s_gaia_type, event_type_t::row_insert, 0}, rule3},
    {{ruleset2_name, rule4_name, TestGaia2::s_gaia_type, event_type_t::row_insert, 0}, rule4},
    {{ruleset1_name, rule1_name, TestGaia2::s_gaia_type, event_type_t::row_update, s_first_name}, rule1},
    {{ruleset1_name, rule2_name, TestGaia2::s_gaia_type, event_type_t::row_update, s_last_name}, rule2}
    //{{ruleset2_name, rule3_name, 0, event_type_t::transaction_begin, 0}, rule3},
    //{{ruleset2_name, rule3_name, 0, event_type_t::transaction_commit, 0}, rule3},
    //{{ruleset2_name, rule4_name, 0, event_type_t::transaction_commit, 0}, rule4},
    //{{ruleset2_name, rule3_name, 0, event_type_t::transaction_rollback, 0}, rule3},
    //{{ruleset2_name, rule4_name, 0, event_type_t::transaction_rollback, 0}, rule4}
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
     subscribe_rule(TestGaia2::s_gaia_type, event_type_t::row_delete, empty_fields, binding);
     unsubscribe_rules();
 }

/**
 * Google test fixture object.  This class is used by each
 * test case below.  SetUp() is called before each test is run
 * and TearDown() is called after each test case is done.
 */
class event_manager_test : public db_test_base_t
{
protected:
    virtual void SetUp() override {
        db_test_base_t::SetUp();
        event_manager_settings_t settings;
        settings.num_background_threads = 0;
        settings.disable_catalog_checks = true;
        test::initialize_rules_engine(settings);
        g_context_checker.get_dummy_transaction(true);
    }

    virtual void TearDown() override {
        unsubscribe_rules();
        g_context_checker.reset(true);
        // This expectation verifies that the caller provided
        // initialize_rules function was called exactly once by
        // the event_manager_t singleton.
        EXPECT_EQ(1, g_initialize_rules_called);
        db_test_base_t::TearDown();
    }

    void validate_rule(
        event_type_t type,
        gaia_type_t gaia_type,
        gaia_id_t record)

    {
        g_context_checker.validate(type, gaia_type, record);
    }

    void add_context_sequence(rule_context_sequence_t& sequence, gaia_type_t gaia_type, event_type_t event_type)
    {
        g_context_checker.add_context_sequence(sequence, gaia_type, event_type);
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
                decl.sub.gaia_type, decl.sub.type, decl.sub.field}));
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
            field_list_t fields;

            if (decl.sub.field)
            {
                fields.insert(decl.sub.field);

            }
            subscribe_rule(gaia_type, event, fields, binding);
        }
    }

    // event log table helpers
    uint64_t clear_event_log()
    {
      uint64_t rows_cleared = 0;
      gaia::db::begin_transaction();
      auto entry = gaia::event_log::event_log_t::get_first();
      while(entry)
      {
          entry.delete_row();
          entry = gaia::event_log::event_log_t::get_first();
          rows_cleared++;
      }
      gaia::db::commit_transaction();
      return rows_cleared;
    }

    void verify_event_log_row(const gaia::event_log::event_log_t& row, event_type_t event_type, uint64_t gaia_type,
        gaia_id_t record_id, uint16_t column_id, bool rules_invoked)
    {
        EXPECT_EQ(row.event_type(), (uint32_t) event_type);
        EXPECT_EQ(row.type_id(), gaia_type);
        EXPECT_EQ(row.record_id(), record_id);
        EXPECT_EQ(row.column_id(), column_id);
        EXPECT_EQ(row.rules_invoked(), rules_invoked);
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
    //rule_binding_t m_rule6{ruleset3_name, rule6_name, rule6};
    rule_binding_t m_rule7{ruleset3_name, rule7_name, rule7};
    rule_binding_t m_rule8{ruleset3_name, rule8_name, rule8};
    rule_binding_t m_rule9{ruleset3_name, rule9_name, rule9};
    rule_binding_t m_rule10{ruleset3_name, rule10_name, rule10};
};

TEST_F(event_manager_test, invalid_subscription)
{
    field_list_t fields;
    fields.insert(1);
    
    // TODO[GAIAPLAT-194]: Transaction Events are out of scope for Q2

    // Transaction event subscriptions can't specify a gaia type
    //EXPECT_THROW(subscribe_rule(TestGaia2::s_gaia_type, event_type_t::transaction_begin, empty_fields, m_rule1), invalid_subscription);
    //EXPECT_THROW(subscribe_rule(TestGaia2::s_gaia_type, event_type_t::transaction_commit, empty_fields, m_rule1), invalid_subscription);
    //EXPECT_THROW(subscribe_rule(TestGaia2::s_gaia_type, event_type_t::transaction_rollback, empty_fields, m_rule1), invalid_subscription);

    // Transactions events subscriptions can't specify fields.
    //EXPECT_THROW(subscribe_rule(0, event_type_t::transaction_begin, fields, m_rule1), invalid_subscription);
    //EXPECT_THROW(subscribe_rule(0, event_type_t::transaction_commit, fields, m_rule1), invalid_subscription);
    //EXPECT_THROW(subscribe_rule(0, event_type_t::transaction_rollback, fields, m_rule1), invalid_subscription);

    // Table delete event cannot specify any fields
    EXPECT_THROW(subscribe_rule(TestGaia2::s_gaia_type, event_type_t::row_delete, fields, m_rule1), invalid_subscription);
    // Table insert cannot specify any fields
    EXPECT_THROW(subscribe_rule(TestGaia2::s_gaia_type, event_type_t::row_insert, fields, m_rule1), invalid_subscription);
}

TEST_F(event_manager_test, log_event_no_rules)
{
    // An empty sequence will verify that the rule was not called.
    rule_context_sequence_t sequence;
    trigger_event_t event = {event_type_t::row_delete, TestGaia::s_gaia_type, 123, nullptr, 0};
    test::commit_trigger(0, &event, 1);
    validate_rule_sequence(sequence);
}

TEST_F(event_manager_test, log_database_event_single_event_single_rule) {
    subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_update, empty_fields, m_rule1);

    // An empty sequence will verify that the rule was not called.
    rule_context_sequence_t sequence;
    add_context_sequence(sequence, TestGaia::s_gaia_type, event_type_t::row_update);

    // fire an insert event and an update event; verify the rule was only fired for update event
    gaia_id_t record = 55;
    trigger_event_t events[] = {
        {event_type_t::row_insert, TestGaia::s_gaia_type, 20, nullptr, 0},
        {event_type_t::row_update, TestGaia::s_gaia_type, record, nullptr, 0}
    };
    test::commit_trigger(0, events, 2);

    validate_rule_sequence(sequence);
    validate_rule(event_type_t::row_update, TestGaia::s_gaia_type, record);
}

TEST_F(event_manager_test, log_field_event_single_event_single_rule) {

    // Ensure we have field level granularity.
    field_list_t fields;
    fields.insert(s_last_name);
    subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_update, fields, m_rule1);
    gaia_id_t record = 999;

    rule_context_sequence_t sequence;
    add_context_sequence(sequence, TestGaia::s_gaia_type, event_type_t::row_update);

    // Verify an update to a different column doesn't fire the rule
    // And then verify an update to the correct column does fire the rule
    trigger_event_t update_field_events[] = {
        {event_type_t::row_update, TestGaia::s_gaia_type, 1, &s_first_name, 1},
        {event_type_t::row_update, TestGaia::s_gaia_type, record, &s_last_name, 1}
    };
    test::commit_trigger(0, update_field_events, 2);
    validate_rule_sequence(sequence);
    validate_rule(event_type_t::row_update, TestGaia::s_gaia_type, record);
}


TEST_F(event_manager_test, log_field_event_multi_event_single_rule) {
    // Ensure we have field level granularity.
    field_list_t fields;

    // Rule 1 will fire on any writes to "last_name" or "first_name"
    fields.insert(s_last_name);
    fields.insert(s_first_name);

    subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_update, fields, m_rule1);

    rule_context_sequence_t sequence;
    add_context_sequence(sequence, TestGaia::s_gaia_type, event_type_t::row_update);

    gaia_id_t record = 30;
    trigger_event_t update_field_event = {event_type_t::row_update, TestGaia::s_gaia_type, record, &s_last_name, 1};
    test::commit_trigger(0, &update_field_event, 1);
    validate_rule_sequence(sequence);
    validate_rule(event_type_t::row_update, TestGaia::s_gaia_type, record);

    add_context_sequence(sequence, TestGaia::s_gaia_type, event_type_t::row_update);
    record = 22;
    update_field_event.columns = &s_first_name;
    update_field_event.record = record;
    test::commit_trigger(0, &update_field_event, 1);
    validate_rule_sequence(sequence);
    validate_rule(event_type_t::row_update, TestGaia::s_gaia_type, record);
}


TEST_F(event_manager_test, log_field_event_multi_event_multi_rule) {
    // Ensure we have field level granularity.
    field_list_t fields;

    // Rule 1 will fire on write to "last_name".
    fields.insert(s_last_name);
    subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_update, fields, m_rule1);

    // Rule 2 will fire on write to "first_name".
    fields.clear();
    fields.insert(s_first_name);
    subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_update, fields, m_rule2);

    rule_context_sequence_t sequence;
    gaia_id_t record = 3;
    add_context_sequence(sequence, TestGaia::s_gaia_type, event_type_t::row_update);

    trigger_event_t update_field_event = {event_type_t::row_update, TestGaia::s_gaia_type, record, &s_last_name, 1};
    test::commit_trigger(0, &update_field_event, 1);

    validate_rule_sequence(sequence);
    validate_rule(event_type_t::row_update, TestGaia::s_gaia_type, record);

    add_context_sequence(sequence, TestGaia::s_gaia_type, event_type_t::row_update);
    update_field_event.columns = &s_first_name;
    test::commit_trigger(0, &update_field_event, 1);

    validate_rule_sequence(sequence);
    validate_rule(event_type_t::row_update, TestGaia::s_gaia_type, record);
}


TEST_F(event_manager_test, log_database_event_single_rule_multi_event)
{
    // Bind same rule to update and insert
    subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_update, empty_fields, m_rule1);
    subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_insert, empty_fields, m_rule1);
    rule_context_sequence_t sequence;
    gaia_id_t record = 8000;
    add_context_sequence(sequence, TestGaia::s_gaia_type, event_type_t::row_update);
    add_context_sequence(sequence, TestGaia::s_gaia_type, event_type_t::row_insert);

    // Log delete, update, and insert.  Sequence should be update followed by insert
    // because we didn't bind a rule to delete.
    trigger_event_t events[] = {
        {event_type_t::row_delete, TestGaia::s_gaia_type, record, nullptr, 0},
        {event_type_t::row_update, TestGaia::s_gaia_type, record+1, nullptr, 0},
        {event_type_t::row_insert, TestGaia::s_gaia_type, record+2, nullptr, 0}
    };
    test::commit_trigger(0, events, 3);
    validate_rule_sequence(sequence);
}


TEST_F(event_manager_test, log_database_event_multi_rule_single_event)
{
    // Bind two rules to the same event.
    subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_delete, empty_fields, m_rule1);
    subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_delete, empty_fields, m_rule2);

    rule_context_sequence_t sequence;
    add_context_sequence(sequence, TestGaia::s_gaia_type, event_type_t::row_delete);
    add_context_sequence(sequence, TestGaia::s_gaia_type, event_type_t::row_delete);

    // Log an update event followed by a delete event.  We should see two rules
    // fired in response to the delete event.
    trigger_event_t events[] = {
        {event_type_t::row_update, TestGaia::s_gaia_type, 1, nullptr, 0},
        {event_type_t::row_delete, TestGaia::s_gaia_type, 100, nullptr, 0},
    };
    test::commit_trigger(0, events, 2);
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

    // Log event for TestGaia::update
    // Log event TestGaia::delete to invoke rule1 and rule2.
    // Log event for commit for rule 3 and 4
    add_context_sequence(sequence, TestGaia::s_gaia_type, event_type_t::row_update);
    add_context_sequence(sequence, TestGaia::s_gaia_type, event_type_t::row_delete);
    add_context_sequence(sequence, TestGaia::s_gaia_type, event_type_t::row_delete);
    
    // TODO[GAIAPLAT-194]: Transaction Events are out of scope for Q2
    //add_context_sequence(sequence, 0, event_type_t::transaction_commit);
    //add_context_sequence(sequence, 0, event_type_t::transaction_commit);
    gaia_id_t record = 100;

    trigger_event_t events[] = {
        {event_type_t::row_update, TestGaia::s_gaia_type, 1, &s_first_name, 1},
        {event_type_t::row_delete, TestGaia::s_gaia_type, record, nullptr, 0}
    //    {event_type_t::transaction_commit, 0, 0, nullptr, 0}
    };
    test::commit_trigger(0, events, 2);
    validate_rule_sequence(sequence);

    // Unsubscribe rule1 from delete
    // Rule 2 gets fired
    field_list_t empty_fields;
    EXPECT_EQ(true, unsubscribe_rule(TestGaia::s_gaia_type, event_type_t::row_delete, empty_fields, m_rule1));

    add_context_sequence(sequence, TestGaia::s_gaia_type, event_type_t::row_delete);

    // now fire row_delete trigger
    test::commit_trigger(0, &events[1], 1);
    validate_rule_sequence(sequence);
    validate_rule(event_type_t::row_delete, TestGaia::s_gaia_type, record);

    // Insert should invoke rule2
    add_context_sequence(sequence, TestGaia::s_gaia_type, event_type_t::row_insert);

    record = 205;
    trigger_event_t single_event = {event_type_t::row_insert, TestGaia::s_gaia_type, record, nullptr, 0};
    test::commit_trigger(0, &single_event, 1);
    validate_rule_sequence(sequence);
    validate_rule(event_type_t::row_insert, TestGaia::s_gaia_type, record);

    // Update should invoke rule1.
    add_context_sequence(sequence, TestGaia::s_gaia_type, event_type_t::row_update);
    single_event.event_type = event_type_t::row_update;
    record++;
    single_event.record = record;
    test::commit_trigger(0, &single_event, 1);

    validate_rule_sequence(sequence);
    validate_rule(event_type_t::row_update, TestGaia::s_gaia_type, record);


    // TODO[GAIAPLAT-194]: Transaction Events are out of scope for Q2

    // Rollback should invoke rule3, rule4.
    //add_context_sequence(sequence, 0, event_type_t::transaction_rollback);
    //add_context_sequence(sequence, 0, event_type_t::transaction_rollback);
    //validate_rule_sequence(sequence);

    // Begin should invoke rule3 only.
    //add_context_sequence(sequence, 0, event_type_t::transaction_begin);
    //trigger_event_t transaction_event = {event_type_t::transaction_begin, 0, 0, nullptr, 0};
    //test::commit_trigger(0, &transaction_event, 1);
    //validate_rule_sequence(sequence);
    //validate_rule(event_type_t::transaction_begin, 0, 0);
}

TEST_F(event_manager_test, subscribe_rule_invalid_rule_binding)
{
    rule_binding_t rb;

    // Empty binding.
    EXPECT_THROW(subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_insert, empty_fields, rb), invalid_rule_binding);

    // No rule_name or rule set.
    rb.ruleset_name = ruleset1_name;
    EXPECT_THROW(subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_update, empty_fields, rb), invalid_rule_binding);

    // No rule set.
    rb.rule_name = rule1_name;
    EXPECT_THROW(subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_delete, empty_fields, rb), invalid_rule_binding);
}

TEST_F(event_manager_test, unsubscribe_rule_invalid_rule_binding)
{
    rule_binding_t rb;

    // Empty binding
    EXPECT_THROW(unsubscribe_rule(TestGaia::s_gaia_type, event_type_t::row_delete, empty_fields, rb), invalid_rule_binding);

    // No rule_name or rule set.
    rb.ruleset_name = ruleset1_name;
    EXPECT_THROW(unsubscribe_rule(TestGaia::s_gaia_type, event_type_t::row_delete, empty_fields, rb), invalid_rule_binding);

    // No rule set.
    rb.rule_name = rule1_name;
    EXPECT_THROW(unsubscribe_rule(TestGaia::s_gaia_type, event_type_t::row_delete, empty_fields, rb), invalid_rule_binding);
}

TEST_F(event_manager_test, subscribe_rule_duplicate_rule)
{
    rule_binding_t rb;
    rb.ruleset_name = ruleset1_name;
    rb.rule = rule1;
    rb.rule_name = rule1_name;

    subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_insert, empty_fields, rb);
    EXPECT_THROW(subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_insert, empty_fields, rb), duplicate_rule);

    // Another case of duplicate rule is if we try to bind
    // the same ruleset_name and rule_name to a different rule.
    // Bind to a different event so that it would have been legal
    // if we didn't check for this condition
    rb.rule = rule2;
    EXPECT_THROW(subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_delete, empty_fields, rb), duplicate_rule);
}

TEST_F(event_manager_test, unsubscribe_rule_not_found)
{
    rule_binding_t rb;
    rb.ruleset_name = ruleset1_name;
    rb.rule_name = rule1_name;
    rb.rule = rule1;

    // Rule not there at all with no rules subscribed.
    EXPECT_EQ(false, unsubscribe_rule(TestGaia::s_gaia_type, event_type_t::row_update, empty_fields, rb));

    // Subscribe a valid rule.
    subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_update, empty_fields, rb);

    // Try to remove the rule from the other table events that we didn't register the rule on.
    EXPECT_EQ(false, unsubscribe_rule(TestGaia::s_gaia_type, event_type_t::row_insert, empty_fields, rb));
    EXPECT_EQ(false, unsubscribe_rule(TestGaia::s_gaia_type, event_type_t::row_delete, empty_fields, rb));

    // Try to remove the rule from a type that we didn't register the rule on
    EXPECT_EQ(false, unsubscribe_rule(TestGaia2::s_gaia_type, event_type_t::row_update, empty_fields, rb));

    // With valid rule registered, now ensure that the rule is not found if we change the rule name
    rb.rule_name = rule2_name;
    rb.rule = rule2;
    EXPECT_EQ(false, unsubscribe_rule(TestGaia::s_gaia_type, event_type_t::row_update, empty_fields, rb));

    // Ensure we don't find the rule if we change the ruleset_name
    rb.ruleset_name = ruleset2_name;
    rb.rule_name = rule1_name;
    rb.rule = rule1;
    EXPECT_EQ(false, unsubscribe_rule(TestGaia::s_gaia_type, event_type_t::row_update, empty_fields, rb));
}

TEST_F(event_manager_test, list_rules_none)
{
    subscription_list_t rules;

    // Verify that the list_subscribed_rules api clears the subscription list the caller
    // passes in.
    rules.push_back(unique_ptr<subscription_t>(new subscription_t({"a", "b", 0,
        event_type_t::row_update, s_first_name})));

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

    // TODO[GAIAPLAT-194]: Transaction Events are out of scope for Q2
    //event_type_t event_filter = event_type_t::transaction_begin;
    //list_subscribed_rules(nullptr, nullptr, &event_filter, nullptr, rules);
    //validate_rule_list(rules, get_expected_subscriptions(nullptr, nullptr, &event_filter));

    event_type_t event_filter = event_type_t::row_insert;
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
    event_filter = event_type_t::row_update;
    gaia_type_filter = TestGaia2::s_gaia_type;
    list_subscribed_rules(ruleset_filter, &gaia_type_filter, &event_filter, nullptr, rules);
    validate_rule_list(rules, get_expected_subscriptions(ruleset_filter,
        &gaia_type_filter, &event_filter));
}

// TODO[GAIAPLAT-194]: Transaction Events are out of scope for Q2

/*
TEST_F(event_manager_test, forward_chain_not_subscribed)
{
    subscribe_rule(0, event_type_t::transaction_commit, empty_fields, m_rule8);

    rule_context_sequence_t expected;
    add_context_sequence(expected, 0, event_type_t::transaction_commit);
    trigger_event_t event = {event_type_t::transaction_commit, 0, 0, nullptr, 0};
    test::commit_trigger(0, &event, 1);
    validate_rule_sequence(expected);
}

TEST_F(event_manager_test, forward_chain_transaction_table)
{
    subscribe_rule(0, event_type_t::transaction_commit, empty_fields, m_rule8);
    subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_update, empty_fields, m_rule5);

    rule_context_sequence_t expected;
    add_context_sequence(expected, 0, event_type_t::transaction_commit);
    add_context_sequence(expected, TestGaia::s_gaia_type, event_type_t::row_update);
    trigger_event_t event = {event_type_t::transaction_commit, 0, 0, nullptr, 0};
    test::commit_trigger(0, &event, 1);

    validate_rule_sequence(expected);
}

TEST_F(event_manager_test, forward_chain_table_transaction)
{
    subscribe_rule(TestGaia2::s_gaia_type, event_type_t::row_update, empty_fields, m_rule6);
    subscribe_rule(0, event_type_t::transaction_commit, empty_fields, m_rule8);

    // Because of forward chaining, we expect the table event
    // and the transaction event to be called even though
    // we only logged the table event here.
    rule_context_sequence_t expected;
    add_context_sequence(expected, TestGaia2::s_gaia_type, event_type_t::row_update);
    add_context_sequence(expected, 0, event_type_t::transaction_commit);
    trigger_event_t events[] = {
        {event_type_t::row_update, TestGaia2::s_gaia_type, 99, nullptr, 0},
    };
    test::commit_trigger(0, events, 1);
    validate_rule_sequence(expected);
}
*/

/*
TEST_F(event_manager_test, forward_chain_disallow_reentrant)
{
    // See section where rules are defined for the rule heirarchy.
    subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_update, empty_fields, m_rule5);

    rule_context_sequence_t expected;
    add_context_sequence(expected, TestGaia::s_gaia_type, event_type_t::row_update);

    trigger_event_t event = {event_type_t::row_update, TestGaia::s_gaia_type, 49, nullptr, 0};
    commit_trigger(0, &event, 1);
    validate_rule_sequence(expected);
}

TEST_F(event_manager_test, forward_chain_disallow_cycle)
{
    // See section where rules are defined for the rule heirarchy.
    // This test creates a cycle where all the rules are subscribed:
    // rule5 -> rule6 -> rule8 -> rule5.
    subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_update, empty_fields, m_rule5);
    subscribe_rule(TestGaia2::s_gaia_type, event_type_t::row_update, empty_fields, m_rule6);
    subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_insert, empty_fields, m_rule7);
    subscribe_rule(0, event_type_t::transaction_commit, empty_fields, m_rule8);

    rule_context_sequence_t expected;
    add_context_sequence(expected, TestGaia::s_gaia_type, event_type_t::row_update);
    add_context_sequence(expected, TestGaia2::s_gaia_type, event_type_t::row_update);
    add_context_sequence(expected, 0, event_type_t::transaction_commit);
    add_context_sequence(expected, TestGaia::s_gaia_type, event_type_t::row_insert);

    trigger_event_t event = {event_type_t::row_update, TestGaia::s_gaia_type, 50, nullptr, 0};
    commit_trigger(0, &event, 1);
    validate_rule_sequence(expected);
}
*/

TEST_F(event_manager_test, forward_chain_field_not_subscribed)
{
    field_list_t fields;
    fields.insert(s_value);
    subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_update, fields, m_rule9);


    // expect only one update from the value field
    rule_context_sequence_t expected;
    add_context_sequence(expected, TestGaia::s_gaia_type, event_type_t::row_update);

    trigger_event_t event = {event_type_t::row_update, TestGaia::s_gaia_type, 34, &s_value, 1};
    test::commit_trigger(0, &event, 1);
    validate_rule_sequence(expected);
}

/*
TEST_F(event_manager_test, forward_chain_field_commit)
{
    field_list_t fields;
    fields.insert(s_value);
    subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_update, fields, m_rule9);
    fields.clear();

    install_transaction_hooks();

    fields.insert(s_timestamp);
    subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_update, fields, m_rule10);

    subscribe_rule(0, event_type_t::transaction_commit, empty_fields, m_rule3);

    // Expect the following sequence of calls.
    rule_context_sequence_t expected;
    add_context_sequence(expected, TestGaia::s_gaia_type, event_type_t::row_update);
    add_context_sequence(expected, 0, event_type_t::transaction_commit);
    add_context_sequence(expected, TestGaia::s_gaia_type, event_type_t::row_update);

    begin_transaction();
    trigger_event_t events[] = {
        {event_type_t::row_update, TestGaia::s_gaia_type, 34, &s_value, 1},
        {event_type_t::transaction_commit, 0, 0, nullptr, 0}
    };
    test::commit_trigger(0, events, 2, false);
    commit_transaction();
    validate_rule_sequence(expected);

    uninstall_transaction_hooks();
}

TEST_F(event_manager_test, forward_chain_field_rollback)
{
    field_list_t fields;
    fields.insert(s_value);
    subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_update, fields, m_rule9);
    fields.clear();

    install_transaction_hooks();

    fields.insert(s_timestamp);
    subscribe_rule(TestGaia::s_gaia_type, event_type_t::row_update, fields, m_rule10);
    subscribe_rule(0, event_type_t::transaction_rollback, empty_fields, m_rule3);

    // Expect the following sequence of calls.
    rule_context_sequence_t expected;
    add_context_sequence(expected, 0, event_type_t::transaction_rollback);

    begin_transaction();
        trigger_event_t event = {event_type_t::row_update, TestGaia::s_gaia_type, 34, &s_value, 1};
        test::commit_trigger(0, &event, 1, false);
    rollback_transaction();
    validate_rule_sequence(expected);

    uninstall_transaction_hooks();
}
*/

TEST_F(event_manager_test, event_logging_no_subscriptions)
{
    clear_event_log();

    gaia_id_t record = 11;

    // Ensure the event was logged even if it had no subscribers.
    trigger_event_t events[] = {
        {event_type_t::row_update, TestGaia::s_gaia_type, record, &s_last_name, 1},
    };
    test::commit_trigger(0, events, 1);

    gaia::db::begin_transaction();
    auto entry = gaia::event_log::event_log_t::get_first();
    verify_event_log_row(entry, event_type_t::row_update, 
        TestGaia::s_gaia_type, record, s_last_name, false);

    EXPECT_FALSE(entry.get_next());
    gaia::db::commit_transaction();

    EXPECT_EQ(1, clear_event_log());
}

TEST_F(event_manager_test, event_logging_subscriptions)
{
    clear_event_log();
    setup_all_rules();

    gaia_id_t record = 7000;

    // Log events with subscriptions and ensure the table is populated.
    trigger_event_t events[] = {
        {event_type_t::row_update, TestGaia2::s_gaia_type, record, &s_first_name, 1},
        {event_type_t::row_insert, TestGaia2::s_gaia_type, record + 1, nullptr, 0},
    };
    test::commit_trigger(0, events, 2);

    gaia::db::begin_transaction();
    auto entry = gaia::event_log::event_log_t::get_first();
    verify_event_log_row(entry, event_type_t::row_update, 
        TestGaia2::s_gaia_type, record, s_first_name, true);

    entry = entry.get_next();
    verify_event_log_row(entry, event_type_t::row_insert,
        TestGaia2::s_gaia_type, record + 1, 0, true);

    gaia::db::commit_transaction();
    EXPECT_EQ(2, clear_event_log());
}
