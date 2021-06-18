// RUN: %clang_cc1  -fsyntax-only -verify -fgaia-extensions %s

ruleset test97 : Table(farmer)
{
    {
        if (farmer->raised) // expected-error {{invalid argument type 'raised__type' to unary expression}}
        {}
    }
}
