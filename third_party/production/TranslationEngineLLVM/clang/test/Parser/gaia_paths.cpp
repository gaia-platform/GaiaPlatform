// RUN: %clang_cc1  -fsyntax-only -verify -fgaia-extensions %s

ruleset test90
{
    on_update(incubator)
    {
        long i = actuator.timestamp;
        long j = /incubator->actuator.timestamp;
    }
}

ruleset test91
{
    on_update(incubator)
    {
        long i = actuator->timestamp; // expected-error {{Field 'timestamp' cannot be used after a '->' in a path. Use a '.' to separate the table name from the field name.}}
        // expected-error@-1 {{use of undeclared identifier 'actuator'}}
    }
}

ruleset test92
{
    on_update(incubator)
    {
        int i = 0;
        if (/incubator->raised->animal->feeding.portion > 10)
        {
            i += portion;
        }
    }
}


ruleset test95
{
    {
        \incubator.min_temp = 0.0; // expected-error {{expected expression}}
    }
}

ruleset test96
{
    {
        if (farmer-->raised) // expected-error {{cannot decrement value of type 'farmer__type'}}
        {}
    }
}

ruleset test97
{
    {
        if (farmer>-raised) // expected-error {{invalid argument type 'raised__type' to unary expression}}
        {}
    }
}

ruleset test98
{
    {
        if (farmer<-raised) // expected-error {{invalid argument type 'raised__type' to unary expression}}
        {}
    }
}

ruleset test99
{
    {
        if (farmer->yield.bushels)
        {}
    }
}

ruleset test100
{
    {
        float m = / f : farmer -> incubator . min_temp ;
    }
}

ruleset test101
{
    {
        farmer->incubator->raised.birthdate=actuator->incubator->sensor.timestamp; // expected-error {{assigning to 'const char *' from incompatible type 'unsigned long long'}}
    }
}

// GAIAPLAT-913 (V1)
#ifdef TEST_FAILURES // GAIAPLAT-913 (V1)
ruleset test102
{
    {
        animal->feeding->yield=5; // expected-error {{no viable overloaded '='}}
    }
}
#endif

ruleset test103
{
    {
        animal.age>actuator.timestamp; // expected-warning {{relational comparison result unused}}
    }
}

ruleset test136
{
    {
        while (/incubator)
        {
        }
    }
}

ruleset test137
{
    {
        while (@incubator->sensor.value)
        {
        }
    }
}

ruleset test138
{
    {
        while (sensor->incubator->farmer->yield->feeding.portion)
        {
        }
    }
}

// GAIAPLAT-821 (fixed)
ruleset test130
{
    {
        min_temp += @incubator->sensor.value;
    }
}

ruleset test131
{
    {
        if (farmer->yield)
        {}
    }
}

// GAIAPLAT-877 (fixed)
ruleset test132
{
    on_insert(animal)
    {
        animal->feeding->portion = 5; // expected-error {{Field 'portion' cannot be used after a '->' in a path. Use a '.' to separate the table name from the field name.}}
        // expected-error@-1 {{use of undeclared identifier 'animal'}}
    }
}

// GAIAPLAT-913 (V1)
// GAIAPLAT-878 (V1)
#ifdef TEST_FAILURES // GAIAPLAT-913 (V1)
// GAIAPLAT-878
ruleset test133
{
    on_insert(animal)
    {
        animal->feeding = 5;
    }
}
#endif

// GAIAPLAT-821 (fixed)
// GAIAPLAT-1172 (fixed)
// GAIAPLAT-1199 (fixed)
ruleset test129
{
    on_update(incubator)
    {
        if (/@incubator.min_temp) // expected-error {{expected expression}}
        {
            int i = 0;
        }
    }
}
