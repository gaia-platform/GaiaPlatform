/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cstdlib>
#include <iostream>
#include <string>

#include "gtest/gtest.h"
#include "gaia_addr_book.h"
#include "db_test_base.hpp"
#include "gaia_iterators.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::addr_book;

class gaia_iterator_test : public db_test_base_t {
protected:
    void SetUp() override
    {
        db_test_base_t::SetUp();
        // More set-up goes after base set-up.
    }

    void TearDown() override
    {
        // More tear-down goes before base tear-down.
        db_test_base_t::TearDown();
    }
};

// Test LegacyIterator conformance
// ================================

// Is the iterator CopyConstructible?
// A test for MoveConstructible is not required because it is a prerequisite
// to be CopyConstructible.
TEST_F(gaia_iterator_test, copy_constructible) {
    EXPECT_TRUE(is_copy_constructible<gaia_iterator_t<employee_t>>::value)
        << "The iterator is not CopyConstructible.";
}

// Is the iterator CopyAssignable?
TEST_F(gaia_iterator_test, copy_assignable) {
    EXPECT_TRUE(is_copy_assignable<gaia_iterator_t<employee_t>>::value)
        << "The iterator is not CopyAssignable.";
}

// Is the iterator Destructible?
TEST_F(gaia_iterator_test, destructible) {
    EXPECT_TRUE(is_destructible<gaia_iterator_t<employee_t>>::value)
        << "The iterator is not Destructible.";
}

// Are iterator lvalues Swappable?
TEST_F(gaia_iterator_test, swappable) {
    EXPECT_TRUE(is_swappable<gaia_iterator_t<employee_t*>>::value)
        << "The iterator is not Swappable as an lvalue.";
}

// Does iterator_traits<gaia_iterator_t<edc*>> have member typedefs
// value_type, difference_type, reference, pointer, and iterator_category?
TEST_F(gaia_iterator_test, iterator_traits) {
    // This test can only fail in compile time.
    iterator_traits<gaia_iterator_t<employee_t>>::value_type vt;
    iterator_traits<gaia_iterator_t<employee_t>>::difference_type dt;
    iterator_traits<gaia_iterator_t<employee_t>>::reference r = vt;
    iterator_traits<gaia_iterator_t<employee_t>>::pointer p;
    iterator_traits<gaia_iterator_t<employee_t>>::iterator_category ic;
    // Perhaps SFINAE can be exploited in the future to detect typedefs and
    // return boolean values for GTest.

    // This stops the compiler from complaining about unused variables.
    (void)vt;
    (void)dt;
    (void)r;
    (void)p;
    (void)ic;
}

// Are iterator lvalues dereferenceable?
TEST_F(gaia_iterator_test, lvalue_dereferenceable) {
    const char* emp_name = "Employee0";

    auto_transaction_t tx;
    auto emp_writer = employee_writer();
    emp_writer.name_first = emp_name;
    emp_writer.insert_row();
    gaia_iterator_t<employee_t> emp_iter = employee_t::list().begin();

    EXPECT_STREQ((*emp_iter).name_first(), emp_name)
        << "The iterator is not dereferenceable as an lvalue with the "
        "expected effects.";
}

// Are iterator lvalues pre-incrementable?
TEST_F(gaia_iterator_test, lvalue_incrementable) {
    const int loops = 10;
    auto_transaction_t tx;

    auto emp_writer = employee_writer();
    for (int i = 0; i < loops; i++)
    {
        string name_str = std::to_string(i);
        const char* emp_name = name_str.c_str();

        emp_writer.name_first = emp_name;
        emp_writer.insert_row();
    }
    tx.commit();

    gaia_iterator_t<employee_t> emp_iter = employee_t::list().begin();
    for(int i = 0; i < loops; i++)
    {
        string name_str = std::to_string(i);
        const char* emp_name = name_str.c_str();

        ASSERT_STREQ((*emp_iter).name_first(), emp_name)
            << "The iterator is not dereferenceable as an lvalue with the "
            "expected effects.";
        ++emp_iter;
    }
    ++emp_iter;
}

// Test LegacyInputIterator conformance
// ================================

// Is the iterator EqualityComparable?
TEST_F(gaia_iterator_test, equality_comparable) {
    const char* emp_name_0 = "Employee0";
    const char* emp_name_1 = "Employee1";
    const char* emp_name_2 = "Employee2";

    auto_transaction_t tx;
    auto emp_writer = employee_writer();

    emp_writer.name_first = emp_name_0;
    emp_writer.insert_row();
    emp_writer.name_first = emp_name_1;
    emp_writer.insert_row();
    emp_writer.name_first = emp_name_2;
    emp_writer.insert_row();

    tx.commit();

    gaia_iterator_t<employee_t> a = employee_t::list().begin();
    gaia_iterator_t<employee_t> b = employee_t::list().begin();
    gaia_iterator_t<employee_t> c = employee_t::list().begin();

    // a == a is always true.
    EXPECT_TRUE(a == a);
    // If a == b then b == a.
    EXPECT_TRUE((a == b) && (b == a));
    // If a == b and b == c then a == c.
    EXPECT_TRUE((a == b) && (b == c) && (a == c));

    ++b;
    c = employee_t::list().end();

    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == c);
    EXPECT_FALSE(a == c);
}
/*
// Does the iterator support the not-equal (!=) operator?
TEST_F(gaia_iterator_test, not_equal) {
    employee_t emp1;
    employee_t emp2;

    gaia_iterator_t<employee_t*> iter1(&emp1);
    gaia_iterator_t<employee_t*> iter2(&emp2);

    cout << (*iter1)->gaia_id() << endl;
    cout << (*iter2)->gaia_id() << endl;
    EXPECT_TRUE(iter1 == iter2);
}

// Is the iterator dereferenceable back to the pointer it was declared at?
TEST_F(gaia_iterator_test, dereferenceable) {
    employee_t* ptr = new employee_t;
    gaia_iterator_t<employee_t*> iter(ptr);

    EXPECT_TRUE((*iter) == ptr);

    delete ptr;
}

// When an iterator is dereferenced, can the object members be accessed?
// Test of the arrow operator (->).
TEST_F(gaia_iterator_test, deref_arrow) {
    EXPECT_TRUE(true)
        << "The class member derefence operator (->) must be implemented.";
}
*/
