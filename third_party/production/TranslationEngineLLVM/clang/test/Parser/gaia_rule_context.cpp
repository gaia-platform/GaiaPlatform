// RUN: %clang_cc1 -fsyntax-only -verify -fgaia-extensions %s

ruleset test_rule_context_1
{
    {
        auto x = rule_context; // expected-error {{Invalid use of 'rule_context'.}}
    }
}

ruleset test_rule_context_2
{
    {
        int rule_context = 5; // expected-error {{expected unqualified-id}}
    }
}

ruleset test_rule_context_3
{
    {
        auto x = rule_context.x; // expected-error {{no member named 'x' in 'rule_context__type'}}
    }
}

ruleset test_rule_context_4
{
    {
        auto x = &rule_context; // expected-error {{Invalid use of 'rule_context'.}}
    }
}

ruleset test_rule_context_5
{
    {
        rule_context.rule_name = "test"; // expected-error {{expression is not assignable}}
    }
}

ruleset test_rule_context_6
{
    {
        rule_context.rule_name[3] = 't'; // expected-error {{read-only variable is not assignable}}
    }
}

ruleset test_rule_context_7
{
    {
        rule_context.event_type = 5; // expected-error {{expression is not assignable}}
    }
}

ruleset test_rule_context_8
{
    {
        rule_context.gaia_type = 5; // expected-error {{expression is not assignable}}
    }
}

ruleset test_rule_context_9
{
    {
        rule_context.ruleset_name = "test"; // expected-error {{expression is not assignable}}
    }
}

ruleset test_rule_context_10
{
    {
        rule_context.ruleset_name[3] = 't'; // expected-error {{read-only variable is not assignable}}
    }
}
