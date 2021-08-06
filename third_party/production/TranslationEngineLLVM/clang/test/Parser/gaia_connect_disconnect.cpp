// RUN: %clang_cc1 -fgaia-extensions -ast-dump -verify %s -verify-ignore-unexpected=note | FileCheck -strict-whitespace %s

ruleset test_connect_disconnect_on_table
{
    OnInsert(farmer)
    {
        for (/r : raised)
        {
            farmer.Connect(r);
            // CHECK:    CXXMemberCallExpr 0x{{[^ ]*}} <{{.*}}> 'bool'
            // CHECK-NEXT:    MemberExpr 0x{{[^ ]*}} <{{.*}}> '<bound member function type>' .Connect 0x{{[^ ]*}}
            // CHECK-NEXT:    DeclRefExpr 0x{{[^ ]*}} <{{.*}}> 'farmer__type' lvalue Var 0x{{[^ ]*}} 'farmer' 'farmer__type'

            farmer.Disconnect(r);
            // CHECK:    CXXMemberCallExpr 0x{{[^ ]*}} <{{.*}}> 'bool'
            // CHECK-NEXT:    MemberExpr 0x{{[^ ]*}} <{{.*}}> '<bound member function type>' .Disconnect 0x{{[^ ]*}}
            // CHECK-NEXT:    DeclRefExpr 0x{{[^ ]*}} <{{.*}}> 'farmer__type' lvalue Var 0x{{[^ ]*}} 'farmer' 'farmer__type'
        }
    }
}

ruleset test_connect_disconnect_on_link
{
    OnInsert(farmer)
    {
        for (/i : incubator)
        {
            farmer.incubators.Connect(i);
            // CHECK:    CXXMemberCallExpr 0x{{[^ ]*}} <{{.*}}> 'bool'
            // CHECK-NEXT:    MemberExpr 0x{{[^ ]*}} <{{.*}}> '<bound member function type>' .Connect 0x{{[^ ]*}}
            // CHECK-NEXT:    MemberExpr 0x{{[^ ]*}} <{{.*}}> 'farmer_incubators__type' lvalue .incubators 0x{{[^ ]*}}
            // CHECK-NEXT:    DeclRefExpr 0x{{[^ ]*}} <{{.*}}> 'farmer__type' lvalue Var 0x{{[^ ]*}} 'farmer' 'farmer__type'

            farmer.incubators.Disconnect(i);
            // CHECK:    CXXMemberCallExpr 0x{{[^ ]*}} <{{.*}}> 'bool'
            // CHECK-NEXT:    MemberExpr 0x{{[^ ]*}} <{{.*}}> '<bound member function type>' .Disconnect 0x{{[^ ]*}}
            // CHECK-NEXT:    MemberExpr 0x{{[^ ]*}} <{{.*}}> 'farmer_incubators__type' lvalue .incubators 0x{{[^ ]*}}
            // CHECK-NEXT:    DeclRefExpr 0x{{[^ ]*}} <{{.*}}> 'farmer__type' lvalue Var 0x{{[^ ]*}} 'farmer' 'farmer__type'

            // This is not possible ATM because of this bug: https://gaiaplatform.atlassian.net/browse/GAIAPLAT-1037
            // In order to correctly tell apart tables from links we need to improve the parsing logic.
  //        incubators.Connect(i);
  //        incubators.Disconnect(i);
        }
    }
}

ruleset test_connect_disconnect_fail_in_isolated_table
{
    OnInsert(isolated)
    {
        int i = 1;
        isolated.Connect(i); // expected-error {{no member named 'Connect' in 'isolated__type'}}
        isolated.Disconnect(i); // expected-error {{no member named 'Disconnect' in 'isolated__type'}}
    }
};

ruleset test_connect_disconnect_invalid_syntax_1
{
    OnInsert(crop)
    {
       crop.Connect(); // expected-error {{no matching member function for call to 'Connect'}}
       crop.Disconnect(); // expected-error {{no matching member function for call to 'Disconnect'}}
       crop.yield.Connect(); // expected-error {{too few arguments to function call, single argument 'param_1' was not specified}}
       crop.yield.Disconnect(); // expected-error {{too few arguments to function call, single argument 'param_1' was not specified}}
       crop.yield.Connect("aaaaa"); // expected-error {{non-const lvalue reference to type 'yield__type' cannot bind to a value of unrelated type 'const char [6]'}}
       crop.yield.Disconnect(1); // expected-error {{non-const lvalue reference to type 'yield__type' cannot bind to a temporary of type 'int'}}
    }
}


ruleset test_connect_disconnect_invalid_syntax_2
{
    OnInsert(farmer)
    {
        farmer.Connect; // expected-error {{reference to non-static member function must be called}}
        farmer.Disconnect; // expected-error {{reference to non-static member function must be called}}
        farmer.incubators.Connect;  // expected-error {{reference to non-static member function must be called}}
        farmer.incubators.Disconnect; // expected-error {{reference to non-static member function must be called}}
    }
}

ruleset test_connect_disconnect_too_many_params
{
    OnInsert(farmer)
    {
        for (/i : incubator)
        {
            // Connect/Disconnect accept one parameter.
            farmer.Connect(i, i); // expected-error {{no matching member function for call to 'Connect'}}
            farmer.Disconnect(i, i); // expected-error {{no matching member function for call to 'Disconnect'}}
        }
    }
}

ruleset test_connect_disconnect_invalid_link
{
    OnInsert(farmer)
    {
        // name is a non-link field (const char *) hence should not expose the connect method.
        farmer.name.Connect(); // expected-error {{member reference base type 'const char *' is not a structure or union}}
        farmer.name.Disconnect(); // expected-error {{member reference base type 'const char *' is not a structure or union}}
    }
}
