// RUN: %clang_cc1  -fsyntax-only -verify -fgaia-extensions %s
// expected-no-diagnostics
ruleset test1
{
    void f(int a[])
    {}
    on_update(isolated)
    {
        isolated.history[5] = 5;
        auto t = isolated.history[5];
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
        f(history);
    }
}
