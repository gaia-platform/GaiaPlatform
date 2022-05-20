// RUN: %clang_cc1 -fsyntax-only -verify -fgaia-extensions %s

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


ruleset test19: tables(sensor)
{
    {
        actuator.value += @value/2; // expected-warning {{Table 'actuator' is not referenced in tables attribute.}}
    }
}

ruleset test15
{
    {
        actuator.value1++; // expected-error {{no member named 'value1' in 'actuator_9f63755fe9e4859c03277e1b6fae0f4e__type'; did you mean 'value'?}} \
                           // expected-note {{'value' declared here}}
    }
}

ruleset test16
{
    {
        value++; // expected-error {{The reference to field 'value'}} \
                 // expected-error {{use of undeclared identifier 'value'}}
    }
}

ruleset test17
{
    {
      x.value++; // expected-error {{Table 'x' was not found in the catalog}} \
                 // expected-error {{use of undeclared identifier 'x'}}
    }
}

ruleset test22
{
    {
        int ruleset = 5; // expected-error {{expected unqualified-id}}
    }
}

ruleset test134
{
    {
        @animal.age = 4;
    }
}
