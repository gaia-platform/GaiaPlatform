// RUN: %clang_cc1  -fsyntax-only -verify -fgaia-extensions %s

// GAIAPLAT-947 (fixed)
ruleset test123
{
    on_insert(incubator)
    {
        int i = 0;
        if (sensor.value > 99.0)
        {
            i = 1;
        }
        else
        {
            i = 2;
        }
        nomatch
        {
            i = 3;
        }
    }
}

ruleset test124
{
    on_insert(incubator)
    {
        int i = 0;
        if (sensor.value > 99.0)
        {
            i = 1;
        }
        nomatch
        {
            i = 3;
        }
    }
}

// GAIAPLAT-948 (fixed)
ruleset test125
{
    on_insert(incubator)
    {
        const char* s = "just a string";
        int i = 0;
        while (*s++)
        {
            i++;
        }
        if (i < 5) // expected-error {{A non-declarative 'if' statement may not use nomatch.}}
        {
            i = 1;
        }
        nomatch   // expected-error {{expected expression}}
        {
            i = 3;
        }
    }
}

ruleset test135
{
    {
        if (animal.age == 0)
        nomatch // expected-error {{expected expression}}
        {
        }
    }
}

ruleset test139
{
    {
        if (@incubator.max_temp > 99.9)
        {
            incubator.max_temp = 99.9;
        }
        nomatch
        {
            /incubator.max_temp = 99.9;
        }
        nomatch // expected-error {{expected expression}}
        {
            /incubator.min_temp = 97.0;
        }
    }
}

ruleset test140
{
    {
        if (@incubator.max_temp > 99.9)
        {
            incubator.max_temp = 99.9;
        }
        else
        {
            if (@incubator.min_temp < 97.0)
            {
                incubator.min_temp = 97.0;
            }
            else
            {
                if (12 > 6 * 3)
                {
                    sensor.value += .03;
                }
            }
            nomatch
            {
                /incubator.min_temp = 99.9;
            }
        }
        nomatch
        {
            /incubator.max_temp = 99.9;
        }
    }
}

ruleset test141
{
    {
        if (@incubator.min_temp < 97.0)
        {}
        nomatch (incubator.max_temp);  // expected-warning {{expression result unused}}
    }
}
