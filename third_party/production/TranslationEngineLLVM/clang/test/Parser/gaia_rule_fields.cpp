// RUN: %clang_cc1 -fsyntax-only -verify -fgaia-extensions %s

// Uncomment the #define to re-test failing tests.
// #define TEST_FAILURES

ruleset test9
{
    {
        int value;
        min_temp += @value; // expected-error {{unexpected '@' in program}}
        max_temp += min_temp / 2;
    }
}

ruleset test10
{
    min_temp += @value; // expected-error {{unknown type name 'min_temp'}} \
                        // expected-error {{expected unqualified-id}}
}


ruleset test19: Table(sensor)
{
    {
        actuator.value += @value/2; // expected-warning {{Table 'actuator' is not referenced in table attribute.}}
    }
}

ruleset test15
{
    {
        actuator.value1++; // expected-error {{no member named 'value1' in 'actuator__type'; did you mean 'value'?}} \
                           // expected-note {{'value' declared here}}
    }
}

ruleset test16
{
    {
        value++; // expected-error {{Ambiguous reference to field 'value'}} \
                 // expected-note {{Field: 'sensor.value'}} \
                 // expected-note {{Field: 'actuator.value'}} \
                 // expected-error {{use of undeclared identifier 'value'}}
    }
}

ruleset test17
{
    {
        x.value++; // expected-error {{Table 'x' was not found in the catalog.}} expected-error {{use of undeclared identifier 'x'}}
    }
}

ruleset test22
{
    {
        int ruleset = 5; // expected-error {{expected unqualified-id}}
    }
}

// GAIAPLAT-803
#ifdef TEST_FAILURES
ruleset test134
{
    {
        @animal.age = 4;
    }
}
#endif

ruleset test_ambiguous_reference_in_path
{
    {
        if (farmer->raised) // expected-error {{use of undeclared identifier 'farmer'}} \
                            // expected-error {{Ambiguous reference to field 'raised'}} \
                            // expected-note {{Table: 'raised'}} \
                            // expected-note {{Link: 'animal.raised'}} \
                            // expected-note {{Link: 'incubator.raised'}} \
                            // expected-note {{Link: 'farmer.raised'}}
        {}
    }
}
