// RUN: %clang_cc1  -fsyntax-only -verify -fgaia-extensions %s

ruleset test1
{
    void f(int a[])
    {}
    on_update(isolated)
    {
        isolated.history[5] = 5;
        history[3] = 2;
        int t = isolated.history[5];
        t = history[3];
        int foo_array[] = {1,2,3};
        history = {3,4,5};
        isolated.history = {6,7,8};
        isolated.history = foo_array;
        history = foo_array;
        isolated.insert(history:foo_array);
        isolated.insert(history:{4,7,8});
        isolated.insert(age:6, history:foo_array);
        isolated.insert(age:7, history:{4,7,8});
        isolated.insert(history:foo_array, age:2);
        isolated.insert(history:{4,7,8}, age:3);
        isolated.history[5]++;
        ++isolated.history[5];
        --history[3];
        history[3]--;
        isolated.history[5]+=5;
        history[3]-=3;
        f(history);
        int test = history; // expected-error {{cannot initialize a variable of type 'int' with an lvalue of type 'int []'}}
        history = 5; // expected-error {{assigning to 'int []' from incompatible type 'int'}}
        history ={3, 4.5}; // expected-error {{type 'double' cannot be narrowed to 'int' in initializer list}}
        // expected-warning@-1 {{implicit conversion from 'double' to 'int' changes value from 4.5 to 4}}
        // expected-note@-2 {{insert an explicit cast to silence this issue}}
        isolated.insert(history:{4,7.7,8}); // expected-error {{Cannot convert from 'initializer list' to 'int []' for parameter 'history'.}}
        isolated.history += {6,7,8}; // expected-error {{invalid operands to binary expression ('int []' and 'int [3]')}}
        isolated.history -= foo_array; // expected-error {{invalid operands to binary expression ('int []' and 'int [3]')}}
    }
}
