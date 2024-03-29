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
        if (farmer-->raised) // expected-error {{cannot decrement value of type 'farmer_97f974881b3726d9a77014b5f3b4d795__type'}}
        {}
    }
}

ruleset test97
{
    {
        if (farmer>-raised) // expected-error {{invalid argument type 'raised_72e22a4abca7ba56c227d97d9d1664c8__type' to unary expression}}
        {}
    }
}

ruleset test98
{
    {
        if (farmer<-raised) // expected-error {{invalid argument type 'raised_72e22a4abca7ba56c227d97d9d1664c8__type' to unary expression}}
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

// Incorrect error message below. The verification is correct by the error has an 'expected-note' that is not
// properly parsed by FileCheck.  The note has no line number and says "(frontend)" instead.
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

ruleset test132
{
    on_insert(animal)
    {
        animal->feeding->portion = 5; // expected-error {{Field 'portion' cannot be used after a '->' in a path. Use a '.' to separate the table name from the field name.}}
        // expected-error@-1 {{use of undeclared identifier 'animal'}}
    }
}

#ifdef TEST_FAILURES // Incorrect error message due to 'expected-note' with no line number.
ruleset test133
{
    on_insert(animal)
    {
        animal->feeding = 5;
    }
}
#endif

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
