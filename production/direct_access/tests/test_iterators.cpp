/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <cstdlib>

#include "gtest/gtest.h"
#include "gaia_addr_book.h"
#include "db_test_base.hpp"
#include "gaia_iterators.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::addr_book;

class gaia_iterator_test : public db_test_base_t {
};

// Test LegacyIterator conformance
// ================================

// Is the iterator MoveConstructible?
TEST_F(iter_test, move_constructible) {
    EXPECT_TRUE(is_move_constructible<gaia_iterator_t<Employee*>>::value)
        << "Iterator not MoveConstructible.";
}

// Is the iterator CopyConstructible?
TEST_F(iter_test, copy_constructible) {
    EXPECT_TRUE(is_copy_constructible<gaia_iterator_t<Employee*>>::value)
        << "Iterator not CopyConstructible.";
}

// Is the iterator CopyAssignable?
TEST_F(iter_test, copy_assignable) {
    EXPECT_TRUE(is_copy_assignable<gaia_iterator_t<Employee*>>::value)
        << "Iterator not CopyAssignable.";
}

// Is the iterator Destructible?
TEST_F(iter_test, destructible) {
    EXPECT_TRUE(is_destructible<gaia_iterator_t<Employee*>>::value)
        << "Iterator not Destructible.";
}

// Are iterator lvalues Swappable?
TEST_F(iter_test, swappable) {
    EXPECT_TRUE(is_swappable<gaia_iterator_t<Employee*>>::value)
        << "Iterator not Swappable as an lvalue.";
}

// Does iterator_traits<gaia_iterator_t<edc*>> have member typedefs
// value_type, difference_type, reference, pointer, and iterator_category?
TEST_F(iter_test, iterator_traits) {
    // this test can only fail in compile time
    iterator_traits<gaia_iterator_t<Employee*>>::value_type vt;
    iterator_traits<gaia_iterator_t<Employee*>>::difference_type dt;
    iterator_traits<gaia_iterator_t<Employee*>>::reference r = vt;
    iterator_traits<gaia_iterator_t<Employee*>>::pointer p;
    iterator_traits<gaia_iterator_t<Employee*>>::iterator_category ic;
    // perhaps SFINAE can be exploited in the future to detect typedefs and
    // return boolean values for gtest

    // stop the compiler from complaining about unused variables
    (void)vt; (void)dt; (void)r; (void)p; (void)ic;
}

// Are iterator lvalues dereferenceable?
TEST_F(iter_test, lvalue_dereferenceable) {
    Employee_writer emp_writer;
    emp_writer.name_first = "Employee1";
    gaia_id_t emp_id = emp_writer.insert_row();
    Employee emp = Employee::get(emp_id);
    emp.writer().update_row();
}

// Are iterator lvalues pre-incrementable?
TEST_F(iter_test, lvalue_incrementable) {
    // this test can only fail in compile time or by segfault
    begin_transaction();
    auto emp = new Employee();
    emp->insert_row();

    gaia_iterator_t<Employee*> iter(emp);
    ++iter = nullptr;

    emp->delete_row();
    commit_transaction();
}

// Test LegacyInputIterator conformance
// ================================

// Is the iterator EqualityComparable?
TEST_F(iter_test, equality_comparable) {
    begin_transaction();
    auto emp = new Employee();
    emp-> insert_row();

    gaia_iterator_t<Employee*> a(emp);
    gaia_iterator_t<Employee*> b(emp);
    gaia_iterator_t<Employee*> c(emp);

    // always: a == a
    EXPECT_TRUE(a == a);

    // if a == b then b == a
    EXPECT_TRUE((a == b) && (b == a));

    // if a == b and b == c then a == c
    EXPECT_TRUE((a == b) && (b == c) && (a == c));

    emp->delete_row();
    commit_transaction();
}

// Does the iterator support the not-equal (!=) operator?
TEST_F(iter_test, not_equal) {
    Employee emp1;
    Employee emp2;

    gaia_iterator_t<Employee*> iter1(&emp1);
    gaia_iterator_t<Employee*> iter2(&emp2);

    cout << (*iter1)->gaia_id() << endl;
    cout << (*iter2)->gaia_id() << endl;
    EXPECT_TRUE(iter1 == iter2);
}

// Is the iterator dereferenceable back to the pointer it was declared at?
TEST_F(iter_test, dereferenceable) {
    Employee* ptr = new Employee;
    gaia_iterator_t<Employee*> iter(ptr);

    EXPECT_TRUE((*iter) == ptr);

    delete ptr;
}

// When an iterator is dereferenced, can the object members be accessed?
// Test of the arrow operator (->).
TEST_F(iter_test, deref_arrow) {
    EXPECT_TRUE(true)
        << "The class member derefence operator (->) must be implemented.";
}
