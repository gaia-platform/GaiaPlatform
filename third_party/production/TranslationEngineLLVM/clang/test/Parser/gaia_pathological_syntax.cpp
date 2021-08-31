// RUN: %clang_cc1  -fsyntax-only -verify -fgaia-extensions %s

// Uncomment the #define to re-test failing tests.

{ // expected-error {{expected unqualified-id}}
    min_temp+=@value;
    max_temp += min_temp/2;
}

// Pathological incorrect syntax cases
ruleset test111 { {.age=5;} } // expected-error {{expected expression}}
ruleset test112 { {->animal.age=5;} } // expected-error {{expected expression}}
ruleset test113 { {animal:.age=5;} } // expected-error {{expected expression}}
ruleset test114 { {animal:=5;} } // expected-error {{expected expression}}
ruleset test115 { {.age=actuator.timestamp;} } // expected-error {{expected expression}}
ruleset test116 { {animal.age=>actuator.timestamp;} } // expected-error {{expected expression}}

#ifdef TEST_FAILURES
ruleset test117 { {animal->.age=5;} } // expected-error {{expected expression}}
ruleset test118 { {3:animal.age=5;} } // expected-error {{expected expression}}
ruleset test119 { {animal[age]=actuator[timestamp];} } // expected-error {{expected expression}}
ruleset test120 { {animal(age)=actuator(timestamp);} } // expected-error {{expected expression}}
ruleset test121 { on_insert(A:animal) {animal.age=age:A;} }  // expected-error {{expected expression}}
#endif
