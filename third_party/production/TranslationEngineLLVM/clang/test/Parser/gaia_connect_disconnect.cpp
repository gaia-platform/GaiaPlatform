// RUN: %clang_cc1 -fgaia-extensions -ast-dump -verify %s | FileCheck -strict-whitespace %s

#include "barn_storage/gaia_barn_storage.h"

ruleset test_connect_disconnect_1
{
    on_insert(farmer)
    {
        gaia::barn_storage::incubator_t i;

        farmer.connect(i);
        // CHECK:    CXXMemberCallExpr 0x{{[^ ]*}} <{{.*}}> 'bool'
        // CHECK-NEXT:    MemberExpr 0x{{[^ ]*}} <{{.*}}> '<bound member function type>' .Connect 0x{{[^ ]*}}
        // CHECK-NEXT:    DeclRefExpr 0x{{[^ ]*}} <{{.*}}> 'farmer__type' lvalue Var 0x{{[^ ]*}} 'farmer' 'farmer__type'

        farmer.disconnect(i);
        // CHECK:    CXXMemberCallExpr 0x{{[^ ]*}} <{{.*}}> 'bool'
        // CHECK-NEXT:    MemberExpr 0x{{[^ ]*}} <{{.*}}> '<bound member function type>' .Disconnect 0x{{[^ ]*}}
        // CHECK-NEXT:    DeclRefExpr 0x{{[^ ]*}} <{{.*}}> 'farmer__type' lvalue Var 0x{{[^ ]*}} 'farmer' 'farmer__type'

        farmer.incubators.connect(i);
        // CHECK:    CXXMemberCallExpr 0x{{[^ ]*}} <{{.*}}> 'bool'
        // CHECK-NEXT:    MemberExpr 0x{{[^ ]*}} <{{.*}}> '<bound member function type>' .Connect 0x{{[^ ]*}}
        // CHECK-NEXT:    MemberExpr 0x{{[^ ]*}} <{{.*}}> 'farmer_incubators__type' lvalue .incubators 0x{{[^ ]*}}
        // CHECK-NEXT:    DeclRefExpr 0x{{[^ ]*}} <{{.*}}> 'farmer__type' lvalue Var 0x{{[^ ]*}} 'farmer' 'farmer__type'

        farmer.incubators.disconnect(i);
        // CHECK:    CXXMemberCallExpr 0x{{[^ ]*}} <{{.*}}> 'bool'
        // CHECK-NEXT:    MemberExpr 0x{{[^ ]*}} <{{.*}}> '<bound member function type>' .Disconnect 0x{{[^ ]*}}
        // CHECK-NEXT:    MemberExpr 0x{{[^ ]*}} <{{.*}}> 'farmer_incubators__type' lvalue .incubators 0x{{[^ ]*}}
        // CHECK-NEXT:    DeclRefExpr 0x{{[^ ]*}} <{{.*}}> 'farmer__type' lvalue Var 0x{{[^ ]*}} 'farmer' 'farmer__type'

        // This is not possible ATM because of this bug: https://gaiaplatform.atlassian.net/browse/GAIAPLAT-1037
        // In order to correctly tell apart tables from links we need to improve the parsing logic.
//        incubators.connect(i);
//        incubators.disconnect(i);
    }
}


ruleset test_connect_disconnect_invalid_syntax
{
    // This is an invalid syntax. Connect()/Disconnect() are supposed to
    // take EDC instances as parameters. We can't enforce this constraint
    // in clang because we don't have access to the EDC types. The check
    // will likely happen inside gaiat.
    //
    // This test just shows an "incorrect" behavior.
    // Greg mentioned there are ways to make the check within SemaGaia.cpp
    // if/when this will happen we can change this test to look for errors.
    on_insert(farmer)
    {
        farmer.connect();
        farmer.disconnect();
        farmer.incubators.connect();
        farmer.incubators.disconnect(1, 2, 3);
    }
}

ruleset test_connect_disconnect_3
{
    on_insert(farmer)
    {
        farmer.name.connect(); // expected-error {{member reference base type 'const char *' is not a structure or union}}
        farmer.name.disconnect(); // expected-error {{member reference base type 'const char *' is not a structure or union}}
    }
}

ruleset test_connect_disconnect_3
{
    on_insert(farmer)
    {
        farmer.Connect; // expected-error {{reference to non-static member function must be called; did you mean to call it with no arguments?}}
        farmer.Disconnect; // expected-error {{reference to non-static member function must be called; did you mean to call it with no arguments?}}
        farmer.incubators.Connect;  // expected-error {{reference to non-static member function must be called; did you mean to call it with no arguments?}}
        farmer.incubators.Disconnect; // expected-error {{reference to non-static member function must be called; did you mean to call it with no arguments?}}
    }
}
