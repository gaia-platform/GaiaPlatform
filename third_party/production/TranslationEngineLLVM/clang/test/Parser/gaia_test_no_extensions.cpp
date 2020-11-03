// RUN: %clang_cc1 %s -fsyntax-only -verify	

ruleset test // expected-error {{unknown type name 'ruleset'}}
{

{
  x+=@z; // expected-error {{expected expression}} 
  // expected-error@-1 {{use of undeclared identifier 'x'}}
  y += x;
}
  
} // expected-error {{expected ';' after top level declarator}}


