/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cstdlib>
#include <iostream>
#include <string>
#include <type_traits>

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

// Tests for LegacyIterator conformance
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

// Are iterators pre-incrementable?
TEST_F(gaia_iterator_test, pre_incrementable) {
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
            << "The iterator is not pre-incrementable with the "
            "expected effects.";
        ++emp_iter;
    }

    // The declaration of type_check will fail in compile-time if the
    // pre-increment operator has the wrong return type.
    gaia_iterator_t<employee_t>& type_check = ++emp_iter;
    (void)type_check;
}

// Tests for LegacyInputIterator conformance
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

    // The declaration of type_check will fail in compile-time if the
    // equality operator's return type cannot be implicitly converted into
    // a bool.
    bool type_check = (a == a);
    (void)type_check;

    // a == a is always true.
    EXPECT_TRUE(a == a);
    // If a == b then b == a.
    EXPECT_TRUE((a == b) && (b == a));
    // If a == b and b == c then a == c.
    EXPECT_TRUE((a == b) && (b == c) && (a == c));

    ++b;
    c = employee_t::list().end();

    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
    EXPECT_FALSE(b == c);
    EXPECT_FALSE(a == c);
}

// Does the iterator support the not-equal (!=) operator?
TEST_F(gaia_iterator_test, not_equal) {
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
    ++b;
    gaia_iterator_t<employee_t> c = employee_t::list().end();

    // The declaration of type_check will fail in compile-time if the
    // not-equal operator's return type cannot be implicitly converted into
    // a bool.
    bool type_check = (a != a);
    (void)type_check;

    EXPECT_FALSE(a != a);
    EXPECT_FALSE(b != b);
    EXPECT_FALSE(c != c);

    EXPECT_TRUE(a != b);
    EXPECT_TRUE(b != a);
    EXPECT_TRUE(b != c);
    EXPECT_TRUE(c != b);
    EXPECT_TRUE(a != c);
    EXPECT_TRUE(c != a);

    EXPECT_TRUE((a != b) == !(a == b));
    EXPECT_TRUE((a != a) == !(a == a));
}

// Are iterators dereferenceable as rvalues?
TEST_F(gaia_iterator_test, dereferenceable_rvalue) {
    const char* emp_name = "Employee0";

    auto_transaction_t tx;
    auto emp_writer = employee_writer();
    emp_writer.name_first = emp_name;
    emp_writer.insert_row();
    gaia_iterator_t<employee_t> emp_iter = employee_t::list().begin();

    EXPECT_STREQ((*emp_iter).name_first(), emp_name)
        << "The iterator is not dereferenceable as an rvalue with the "
        "expected effects.";
}

// If two rvalue iterators are equal then their dereferenced values are equal.
TEST_F(gaia_iterator_test, dereferenceable_equality) {
    const char* emp_name = "Employee0";
    auto_transaction_t tx;
    auto emp_writer = employee_writer();
    emp_writer.name_first = emp_name;
    gaia_id_t emp_id = emp_writer.insert_row();
    tx.commit();

    employee_t employee = employee_t::get(emp_id);
    gaia_iterator_t<employee_t> emp_iter_a = employee_t::list().begin();
    gaia_iterator_t<employee_t> emp_iter_b = employee_t::list().begin();
    ASSERT_TRUE(emp_iter_a == emp_iter_b);

    EXPECT_TRUE(*emp_iter_a == *emp_iter_b);
}

// When an iterator is dereferenced, can the object members be accessed?
// Test of the arrow operator (->).
TEST_F(gaia_iterator_test, deref_arrow) {
    const char* emp_name = "Employee0";

    auto_transaction_t tx;
    auto emp_writer = employee_writer();
    emp_writer.name_first = emp_name;
    emp_writer.insert_row();
    gaia_iterator_t<employee_t> emp_iter = employee_t::list().begin();

    EXPECT_STREQ(emp_iter->name_first(), emp_name)
        << "The class member derefence operator->() does not work.";
}

// Does (void)iter++ have the same effect as (void)++iter?
TEST_F(gaia_iterator_test, pre_inc_and_post_inc) {
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

    gaia_iterator_t<employee_t> emp_iter_a = employee_t::list().begin();
    gaia_iterator_t<employee_t> emp_iter_b = employee_t::list().begin();

    (void)++emp_iter_a;
    (void)emp_iter_b++;

    EXPECT_TRUE(emp_iter_a == emp_iter_b)
        << "(void)++iter and (void)iter++ have different effects.";
    EXPECT_STREQ(emp_iter_a->name_first(), emp_iter_b->name_first())
        << "(void)++iter and (void)iter++ have different effects.";

    (void)++emp_iter_a;
    (void)emp_iter_b++;

    EXPECT_EQ(emp_iter_a, emp_iter_b)
        << "(void)++iter and (void)iter++ have different effects.";
    EXPECT_STREQ(emp_iter_a->name_first(), emp_iter_b->name_first())
        << "(void)++iter and (void)iter++ have different effects.";
}

// Does derefencing and postincrementing *iter++ have the expected effects?
TEST_F(gaia_iterator_test, deref_and_postinc) {
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

    gaia_iterator_t<employee_t> emp_iter_a = employee_t::list().begin();
    gaia_iterator_t<employee_t> emp_iter_b = employee_t::list().begin();

    employee_t employee = *emp_iter_b;
    ++emp_iter_b;
    EXPECT_EQ((*emp_iter_a++).name_first(), employee.name_first())
        << "*iter++ does not have the expected effects.";

    employee = *emp_iter_b;
    ++emp_iter_b;
    EXPECT_EQ((*emp_iter_a++).name_first(), employee.name_first())
        << "*iter++ does not have the expected effects.";

    employee = *emp_iter_b;
    ++emp_iter_b;
    EXPECT_EQ((*emp_iter_a++).name_first(), employee.name_first())
        << "*iter++ does not have the expected effects.";
}

// Tests for LegacyForwardIterator conformance
// ================================

// Is the iterator DefaultConstructible?
TEST_F(gaia_iterator_test, default_constructible) {
    EXPECT_TRUE(is_default_constructible<gaia_iterator_t<employee_t>>::value)
        << "The iterator is not DefaultConstructible.";
}

// Is equality and inequality defined over all iterators for the same
// underlying sequence?
TEST_F(gaia_iterator_test, equality_and_inequality_in_sequence) {
    auto_transaction_t tx;
    auto emp_writer = employee_writer();

    for (int i = 0; i < 10; i++)
    {
        emp_writer.ssn = to_string(i).c_str();
        emp_writer.insert_row();
    }
    tx.commit();

    gaia_iterator_t<employee_t> emp_iter_a = employee_t::list().begin();
    for (gaia_iterator_t<employee_t> emp_iter_b = employee_t::list().begin();
            emp_iter_b != employee_t::list().end(); ++emp_iter_b)
    {
        ASSERT_TRUE(emp_iter_a == emp_iter_b)
            << "Equality comparisons are not defined across all iterators in the same sequence.";
        ++emp_iter_a;
    }

    for (gaia_iterator_t<employee_t> emp_iter = employee_t::list().begin();
            emp_iter != employee_t::list().end(); ++emp_iter)
    {
        if (emp_iter == employee_t::list().begin())
        {
            ASSERT_TRUE(emp_iter != employee_t::list().end())
                << "Inequality comparisons are not defined across all iterators in the same sequence.";
        }
        else
        {
            ASSERT_TRUE(emp_iter != employee_t::list().begin())
                << "Inequality comparisons are not defined across all iterators in the same sequence.";
        }
    }
}

// Does post-incrementing the iterator have the expected effects?
TEST_F(gaia_iterator_test, post_increment) {
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

    gaia_iterator_t<employee_t> emp_iter_a = employee_t::list().begin();
    gaia_iterator_t<employee_t> emp_iter_b = employee_t::list().begin();

    EXPECT_TRUE(emp_iter_a++ == emp_iter_b);
    emp_iter_b++;
    EXPECT_TRUE(emp_iter_a++ == emp_iter_b);
    emp_iter_b++;
    EXPECT_TRUE(emp_iter_a == emp_iter_b);
}

// Can an iterator iterate over a sequence multiple times to return the same
// values at the same positions every time? This is known as the multipass
// guarantee.
TEST_F(gaia_iterator_test, multipass_guarantee) {
    
}
