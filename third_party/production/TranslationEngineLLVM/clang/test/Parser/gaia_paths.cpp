// RUN: %clang_cc1  -fsyntax-only -verify -fgaia-extensions %s

// Uncomment the #define to re-test failing tests.
// #define TEST_FAILURES

ruleset test90
{
    OnUpdate(incubator)
    {
        long i = actuator.timestamp;
        long j = /incubator->actuator.timestamp;
    }
}

ruleset test91
{
    OnUpdate(incubator)
    {
        long i = actuator->timestamp; // expected-error {{Tag refers to an invalid table 'timestamp'.}}
        // expected-error@-1 {{use of undeclared identifier 'actuator'}}
    }
}

ruleset test92
{
    OnUpdate(incubator)
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

ruleset test96 : Table(farmer)
{
    {
        if (farmer-->raised) // expected-error {{cannot decrement value of type 'farmer__type'}}
        {}
    }
}

ruleset test97 : Table(farmer)
{
    {
        if (farmer>-raised) // expected-error {{invalid argument type 'farmer_raised__type' to unary expression}}
        {}
    }
}

ruleset test98 : Table(farmer)
{
    {
        if (farmer<-raised) // expected-error {{invalid argument type 'farmer_raised__type' to unary expression}}
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

// GAIAPLAT-913
#ifdef TEST_FAILURES
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

ruleset test136 : Table(incubator)
{
    {
        while (/incubator) // expected-error {{value of type 'incubator__type' is not contextually convertible to 'bool'}}
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

// GAIAPLAT-822
ruleset test131
{
    {
        if (farmer->yield.bushels)
        {}
    }
}

// GAIAPLAT-877
#ifdef TEST_FAILURES
ruleset test132
{
    OnInsert(animal)
    {
        animal->feeding->portion = 5;
    }
}
#endif

// GAIAPLAT-878
// GAIAPLAT-913
#ifdef TEST_FAILURES
ruleset test133
{
    OnInsert(animal)
    {
        animal->feeding = 5;
    }
}
#endif

// GAIAPLAT-821 (fixed)
ruleset test129
{
    OnUpdate(incubator)
    {
        if (/@incubator.min_temp)
        {
            int i = 0;
        }
    }
}
