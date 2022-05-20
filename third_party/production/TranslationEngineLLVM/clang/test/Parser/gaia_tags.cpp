// RUN: %clang_cc1  -fsyntax-only -verify -fgaia-extensions %s

ruleset test80
{
    on_insert(S:sensor)
    {
        actuator.value += S.value/2;
    }
}

ruleset test81
{
    on_update(S:sensor)
    {
        float v = S.value;
    }
}

ruleset test82
{
    on_update(S:sensor, V:sensor.value)
    {
        float v = S.value + V.value;
    }
}


// The 'value' is not duplicated, but qualified by 'sensor'.
// GAIALAT-796 (fixed)
ruleset test83 : tables(sensor)
{
    on_update(value)
    {
        float v;
        v = value * 2.0;
    }
}

ruleset test84
{
    on_insert(I::incubator) // expected-error {{expected ')'}} expected-note {{to match this '('}}
    {
    }
}

ruleset test85
{
    on_insert(I:incubator:sensor) // expected-error {{expected ')'}} expected-note {{to match this '('}})
    {
    }
}

ruleset test86
{
    on_insert(incubator:) // expected-error {{expected identifier}}
    {
    }
}

ruleset test87
{
    on_insert(incubator:I) // expected-error {{Tag 'incubator' cannot have the same name as a table or a field.}}
    {
    }
}


ruleset test89 : tables(actuator)
{
    on_update(A:actuator)
    {
    }
}

// The I.min_temp doesn't use the tag from the 'if'.
ruleset test126
{
    on_change(I:incubator)
    {
        if (/I:incubator.max_temp == 100.0) // expected-error {{Tag 'I' is already defined.}}
        // expected-error@-1  {{use of undeclared identifier 'I'}}
        {
            I.min_temp ++;
        }
    }
}

ruleset test127
{
    on_change(actuator)
    {
        if (/I:incubator.max_temp == 100.0)
        {
            I:incubator.min_temp ++; // expected-error {{Tag 'I' is already defined.}}
            // expected-error@-1  {{use of undeclared identifier 'incubator'}}
        }
    }
}

ruleset test128
{
    on_change(actuator)
    {
        if (/I:incubator.max_temp == 100.0)
        {
            I.min_temp ++;
        }
        I.max_temp++; // expected-error {{Table 'I' was not found in the catalog}}
        // expected-error@-1  {{use of undeclared identifier 'I'}}
    }
}

ruleset test122
{
    on_change(sensor.value)
    {
        if (S:sensor.value > 100.0)
        {
            actuator.value = 101.0;
        }
    }
}
