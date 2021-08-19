// RUN: %clang_cc1 -fgaia-extensions -ast-dump -verify %s -verify-ignore-unexpected=note

#include "barn_storage/gaia_barn_storage.h"

ruleset test_connect_disconnect_on_table
{
    on_insert(farmer)
    {
        for (/r : raised)
        {
            farmer.connect(r);
            farmer.disconnect(r);
        }

        farmer.connect(raised.insert(birthdate: "2 Aug 1990"));

        auto birthday = raised.insert(birthdate: "2 Aug 1990");
        farmer.connect(birthday);
        farmer.disconnect(birthday);
    }
}

ruleset test_connect_disconnect_on_link
{
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

        // This works because incubator_t is available hence the translation engine is able to
        // convert incubator__type into incubator_t.
        farmer.incubators.connect(incubator.insert(name: "Zombies"));

        // Not putting any checks here because for some reason it does not work...
        auto birthday = raised.insert(birthdate: "2 Aug 1990");
        farmer.raised.connect(birthday);
        farmer.raised.disconnect(birthday);
    }
}

ruleset test_connect_disconnect_fail_in_isolated_table
{
    on_insert(isolated)
    {
        int i = 1;
        isolated.connect(i); // expected-error {{no member named 'connect' in 'isolated__type'}}
        isolated.disconnect(i); // expected-error {{no member named 'disconnect' in 'isolated__type'}}
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
};

ruleset test_connect_disconnect_invalid_syntax_1
{
    on_insert(crop)
    {
       crop.connect(); // expected-error {{no matching member function for call to 'connect'}}
       crop.disconnect(); // expected-error {{no matching member function for call to 'disconnect'}}
       crop.yield.connect(); // expected-error {{too few arguments to function call, single argument 'param_1' was not specified}}
       crop.yield.disconnect(); // expected-error {{too few arguments to function call, single argument 'param_1' was not specified}}
       crop.yield.connect("aaaaa"); // expected-error {{reference to type 'const yield__type' could not bind to an lvalue of type 'const char [6]'}}
       crop.yield.disconnect(1); // expected-error {{reference to type 'const yield__type' could not bind to an rvalue of type 'int'}}
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
