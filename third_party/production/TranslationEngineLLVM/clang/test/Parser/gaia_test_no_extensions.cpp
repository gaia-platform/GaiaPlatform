// RUN: %clang_cc1 %s -fsyntax-only -verify	

ruleset test // expected-error {{unknown type name 'ruleset'}}
{

{
  x+=@z; // expected-error {{expected expression}} 
  // expected-error@-1 {{use of undeclared identifier 'x'}}
  y += x;
}
  
} // expected-error {{expected ';' after top level declarator}}

// Unextended 'if' behaves normally.
void function1(int arg)
{
    if (arg > 1)
    {
        arg--;
    }
}

// Unextended 'for' behaves normally.
void function2(char* name)
{
    int sum = 0;
    for (auto i=0; i<10; i++)
    {
        sum += name[i];
    }
}

// Unextended 'while' behaves normally.
void function3(char* name)
{
    int sum = 0;
    int i = 10;
    while (i > 0)
    {
        sum += name[i];
    }
}

// Extended 'if' fails
void function4(int arg)
{
    if (@value > 0) // expected-error {{expected expression}}
    {
        value--; // expected-error {{use of undeclared identifier 'value'}}
    }
}
