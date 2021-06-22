// RUN: %clang_cc1 -fgaia-extensions -ast-dump -verify %s | FileCheck -strict-whitespace %s

#include "barn_storage/gaia_barn_storage.h"

ruleset test_connect_disconnect_1
{
    OnInsert(farmer)
    {
        gaia::barn_storage::incubator_t i;
        farmer.Connect(i);
        // CHECK:    CXXMemberCallExpr 0x{{[^ ]*}} <{{.*}}> 'bool'
        // CHECK-NEXT:    MemberExpr 0x{{[^ ]*}} <{{.*}}> '<bound member function type>' .Connect 0x{{[^ ]*}}
        // CHECK-NEXT:    DeclRefExpr 0x{{[^ ]*}} <{{.*}}> 'farmer__type' lvalue Var 0x{{[^ ]*}} 'farmer' 'farmer__type'

        farmer.Disconnect(i);
        // CHECK:    CXXMemberCallExpr 0x{{[^ ]*}} <{{.*}}> 'bool'
        // CHECK-NEXT:    MemberExpr 0x{{[^ ]*}} <{{.*}}> '<bound member function type>' .Disconnect 0x{{[^ ]*}}
        // CHECK-NEXT:    DeclRefExpr 0x{{[^ ]*}} <{{.*}}> 'farmer__type' lvalue Var 0x{{[^ ]*}} 'farmer' 'farmer__type'

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
    }
}

ruleset test_connect_disconnect_2
{
    // We do not check the parameters passed to connect/disconnect at parse time
    // because we don't have access to the EDC types.
    // The check happens inside gaiat. This test just show this behavior.
    OnInsert(farmer)
    {
        farmer.Connect();
        farmer.Disconnect();
        farmer.incubators.Connect();
        farmer.incubators.Disconnect(1, 2, 3);
    }
}

ruleset test_connect_disconnect_3
{
    OnInsert(farmer)
    {
        farmer.name.Connect(); // expected-error {{member reference base type 'const char *' is not a structure or union}}
        farmer.name.Disconnect(); // expected-error {{member reference base type 'const char *' is not a structure or union}}
    }
}

ruleset test_connect_disconnect_3
{
    OnInsert(farmer)
    {
        farmer.Connect; // expected-error {{reference to non-static member function must be called; did you mean to call it with no arguments?}}
        farmer.Disconnect; // expected-error {{reference to non-static member function must be called; did you mean to call it with no arguments?}}
        farmer.incubators.Connect;  // expected-error {{reference to non-static member function must be called; did you mean to call it with no arguments?}}
        farmer.incubators.Disconnect; // expected-error {{reference to non-static member function must be called; did you mean to call it with no arguments?}}
    }
}

