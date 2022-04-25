// RUN: %clang_cc1  -fsyntax-only -verify -fgaia-extensions %s -verify-ignore-unexpected=note

#include "barn_storage/gaia_barn_storage.h"

ruleset test_connect_disconnect_1_n
{
    on_insert(farmer)
    {
        for (/r : raised)
        {
            farmer.connect(r);
            farmer.disconnect(r);
            r.connect(farmer);
        }

        farmer.connect(raised.insert(birthdate: "2 Aug 1990"));

        auto birthday = raised.insert(birthdate: "2 Aug 1990");
        farmer.connect(birthday);
        farmer.disconnect(birthday);

        birthday.connect(farmer);

        gaia::barn_storage::raised_t r2;
        farmer.connect(r2);
    }

    on_insert(farmer)
    {
        for (/i : incubator)
        {
            farmer.incubators.connect(i);
            farmer.incubators.disconnect(i);

            // This is not possible ATM because of this bug: https://gaiaplatform.atlassian.net/browse/GAIAPLAT-1037
            // In order to correctly tell apart tables from links we need to improve the parsing logic.
  //        incubators.connect(i);
  //        incubators.disconnect(i);
        }

        farmer.incubators.connect(incubator.insert(name: "Zombies"));

        // Not putting any checks here because for some reason it does not work...
        auto r1 = raised.insert(birthdate: "2 Aug 1990");
        farmer.raised.connect(r1);
        farmer.raised.disconnect(r1);

        gaia::barn_storage::raised_t r2;
        farmer.raised.connect(r2);
    }
}

ruleset test_connect_disconnect_1_1
{
    on_insert(animal)
    {
        animal.connect(raised.insert(birthdate: "2 Aug 1990"));

        auto r1 = raised.insert(birthdate: "2 Aug 1990");
        animal.connect(r1);

        gaia::barn_storage::raised_t r2;
        animal.connect(r2);

        // Note: it is not possible to call disconnect
        // directly on a table for 1:1 relationships.
    }

    on_insert(animal)
    {
        animal.raised.connect(raised.insert(birthdate: "2 Aug 1990"));

        auto r1 = raised.insert(birthdate: "2 Aug 1990");
        animal.raised.connect(r1);

        gaia::barn_storage::raised_t r2;
        animal.raised.connect(r2);

        animal.raised.disconnect();
    }
}

ruleset test_connect_disconnect_fail_in_isolated_table
{
    on_insert(isolated)
    {
        int i = 1;
        isolated.connect(i); // expected-error {{no member named 'connect' in 'isolated_220226394582d7117410e3c021748c2a__type'}}
        isolated.disconnect(i); // expected-error {{no member named 'disconnect' in 'isolated_220226394582d7117410e3c021748c2a__type'}}
    }
};

ruleset test_connect_disconnect_fail_with_wrong_param_types
{
    on_insert(farmer)
    {
        for (/i : isolated)
        {
            // I believe the difference in messages between the table and the links is that in the link type we have only one override
            // while in the table we have multiple.
            farmer.connect(i); // expected-error {{no matching member function for call to 'connect'}}
            farmer.disconnect(i); // expected-error {{no matching member function for call to 'disconnect'}}
            farmer.incubators.connect(i); // expected-error {{no matching member function for call to 'connect'}}
            farmer.incubators.disconnect(i); // expected-error {{no matching member function for call to 'disconnect'}}
        }
    }

    on_insert(animal)
    {
        auto r1 = raised.insert(birthdate: "2 Aug 1990");
        animal.raised.disconnect(r1); // expected-error {{too many arguments to function call, expected 0, have 1}}
    }
};

ruleset test_connect_disconnect_invalid_syntax_1
{
    on_insert(incubator)
    {
       incubator.connect(); // expected-error {{no matching member function for call to 'connect'}}
       incubator.disconnect(); // expected-error {{no matching member function for call to 'disconnect'}}
       incubator.sensors.connect(); // expected-error {{too few arguments to function call, single argument 'param_1' was not specified}}
       incubator.sensors.disconnect(); // expected-error {{too few arguments to function call, single argument 'param_1' was not specified}}
       incubator.sensors.connect("aaaaa"); // expected-error {{reference to type 'const sensor_a5fe26d5d09b736a77f4345e9f80b951__type' could not bind to an lvalue of type 'const char [6]'}}
       incubator.sensors.disconnect(1); // expected-error {{reference to type 'const sensor_a5fe26d5d09b736a77f4345e9f80b951__type' could not bind to an rvalue of type 'int'}}
    }
}


ruleset test_connect_disconnect_invalid_syntax_2
{
    on_insert(farmer)
    {
        farmer.connect; // expected-error {{reference to non-static member function must be called}}
        farmer.disconnect; // expected-error {{reference to non-static member function must be called}}
        farmer.incubators.connect;  // expected-error {{reference to non-static member function must be called}}
        farmer.incubators.disconnect; // expected-error {{reference to non-static member function must be called}}
    }
}

ruleset test_connect_disconnect_too_many_params
{
    on_insert(farmer)
    {
        for (/i : incubator)
        {
            // connect/disconnect accept one parameter.
            farmer.connect(i, i); // expected-error {{no matching member function for call to 'connect'}}
            farmer.disconnect(i, i); // expected-error {{no matching member function for call to 'disconnect'}}
        }
    }
}

ruleset test_connect_disconnect_invalid_link
{
    on_insert(farmer)
    {
        // name is a non-link field (const char *) hence should not expose the connect method.
        farmer.name.connect(); // expected-error {{member reference base type 'const char *' is not a structure or union}}
        farmer.name.disconnect(); // expected-error {{member reference base type 'const char *' is not a structure or union}}
    }
}

// This is not a feature but a limitation of the current implementation.
// https://gaiaplatform.atlassian.net/browse/GAIAPLAT-1190
ruleset test_connect_disconnect_works_only_on_parent
{
    on_insert(incubator)
    {
        auto f1 = farmer.insert(name: "Gino D'Acampo");
        incubator.landlord.connect(f1); // expected-error {{no member named 'landlord' in 'incubator_7ef2a3e809cfbd83be97d22711284407__type'}}
        incubator.connect(f1); // expected-error {{no matching member function for call to 'connect'}}
        incubator.disconnect(f1); // expected-error {{no matching member function for call to 'disconnect'}}
    }
}
