/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cstdlib>

#include <algorithm>
#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_set>

#include "gtest/gtest.h"

#include "gaia/direct_access/edc_iterators.hpp"
#include "db_catalog_test_base.hpp"
#include "gaia_addr_book.h"

using namespace std;
using namespace gaia::db;
using namespace gaia::direct_access;
using namespace gaia::common;
using namespace gaia::addr_book;

template <typename T_iterator>
class iterator_conformance_t : public db_catalog_test_base_t
{
public:
    iterator_conformance_t()
        : db_catalog_test_base_t("addr_book.ddl"){};

    void insert_records(size_t count)
    {
        employee_writer employee_writer;
        address_writer address_writer;

        employee_writer.name_first = "Many";
        employee_writer.name_last = "Addresses";
        m_employee = employee_t::get(employee_writer.insert_row());

        for (size_t i = 0; i < count; i++)
        {
            address_writer.street = to_string(i);
            address_t address = address_t::get(address_writer.insert_row());
            m_employee.addressee_address_list().insert(address);
        }
    }

    // Use operator overloading to call the right begin and end methods.  Ignore
    // the parameter, however.
    T_iterator get_begin(gaia_iterator_t<address_t>&)
    {
        return address_t::list().begin();
    }

    T_iterator get_begin(gaia_set_iterator_t<address_t>&)
    {
        return m_employee.addressee_address_list().begin();
    }

    T_iterator get_end(gaia_iterator_t<address_t>&)
    {
        return address_t::list().end();
    }

    T_iterator get_end(gaia_set_iterator_t<address_t>&)
    {
        return m_employee.addressee_address_list().end();
    }

private:
    employee_t m_employee;
};

// Set up the test suite to test the gaia_iterator and gaia_set_iterator types.
using iterator_types
    = ::testing::Types<gaia_iterator_t<address_t>, gaia_set_iterator_t<address_t>>;
TYPED_TEST_SUITE(iterator_conformance_t, iterator_types);

// Tests for LegacyIterator conformance
// ================================

// Is the iterator CopyConstructible?
// A test for MoveConstructible is not required because it is a prerequisite
// to be CopyConstructible.
TYPED_TEST(iterator_conformance_t, copy_constructible)
{
    EXPECT_TRUE(is_copy_constructible<TypeParam>::value) << "The iterator is not CopyConstructible.";
}

// Is the iterator CopyAssignable?
TYPED_TEST(iterator_conformance_t, copy_assignable)
{
    EXPECT_TRUE(is_copy_assignable<TypeParam>::value) << "The iterator is not CopyAssignable.";
}

// Is the iterator Destructible?
TYPED_TEST(iterator_conformance_t, destructible)
{
    EXPECT_TRUE(is_destructible<TypeParam>::value) << "The iterator is not Destructible.";
}

// Are iterator lvalues Swappable?
TYPED_TEST(iterator_conformance_t, swappable)
{
    EXPECT_TRUE(is_swappable<TypeParam>::value) << "The iterator is not Swappable as an lvalue.";
}

// Does iterator_traits<gaia_iterator_t<edc*>> have member typedefs
// value_type, difference_type, reference, pointer, and iterator_category?
TYPED_TEST(iterator_conformance_t, iterator_traits)
{
    // This test can only fail in compile time.
    typename iterator_traits<TypeParam>::value_type vt;
    typename iterator_traits<TypeParam>::difference_type dt;
    typename iterator_traits<TypeParam>::reference r = vt;
    typename iterator_traits<TypeParam>::pointer p;
    typename iterator_traits<TypeParam>::iterator_category ic;

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
TYPED_TEST(iterator_conformance_t, pre_incrementable)
{
    const char* pre_inc_error = "The iterator is not pre-incrementable with the expected effects.";
    const size_t c_loops = 10;
    auto_transaction_t tx;
    this->insert_records(c_loops);

    TypeParam it = this->get_begin(it);
    unordered_set<const char*> set;

    const char* a = (*it).street();
    const char* b = (*++it).street();
    EXPECT_STRNE(a, b) << pre_inc_error;

    it = this->get_begin(it);
    for (size_t i = 0; i < c_loops; i++)
    {
        EXPECT_TRUE(set.find((*it).street()) == set.end()) << pre_inc_error;
        set.insert((*it).street());
        ++it;
    }

    EXPECT_EQ(set.size(), c_loops);

    // The declaration of end_record will fail in compile-time if the
    // pre-increment operator has the wrong return type.
    TypeParam end_record;
    this->get_end(end_record);
    EXPECT_EQ(++it, end_record) << pre_inc_error;
}

// Tests for LegacyInputIterator conformance
// ================================

// Is the iterator EqualityComparable?
TYPED_TEST(iterator_conformance_t, equality_comparable)
{
    auto_transaction_t tx;
    this->insert_records(3);

    TypeParam a = this->get_begin(a);
    TypeParam b = this->get_begin(b);
    TypeParam c = this->get_begin(c);

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
    c = this->get_end(c);

    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
    EXPECT_FALSE(b == c);
    EXPECT_FALSE(a == c);
}

// Does the iterator support the not-equal (!=) operator?
TYPED_TEST(iterator_conformance_t, not_equal)
{
    auto_transaction_t tx;
    this->insert_records(3);

    TypeParam a = this->get_begin(a);
    TypeParam b = this->get_begin(b);
    TypeParam c = this->get_end(c);
    ++b;

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

// Is the reference iterator trait convertible into the value_type iterator
// trait?
TYPED_TEST(iterator_conformance_t, reference_convertibility)
{
    typedef typename iterator_traits<TypeParam>::reference from_t;
    typedef typename iterator_traits<TypeParam>::value_type to_t;

    bool convertible = is_convertible<from_t, to_t>::value;

    EXPECT_TRUE(convertible) << "The reference iterator trait is not convertible into the value_type iterator trait.";
}

// Are iterators dereferenceable as rvalues?
TYPED_TEST(iterator_conformance_t, dereferenceable_rvalue)
{
    auto_transaction_t tx;
    this->insert_records(1);

    TypeParam it = this->get_begin(it);
    EXPECT_EQ(string((*it).street()), to_string(0))
        << "The iterator is not dereferenceable as an rvalue with the expected effects.";
}

// If two rvalue iterators are equal then their dereferenced values are equal.
TYPED_TEST(iterator_conformance_t, dereferenceable_equality)
{
    auto_transaction_t tx;
    this->insert_records(1);

    TypeParam iter_a = this->get_begin(iter_a);
    TypeParam iter_b = this->get_begin(iter_b);
    EXPECT_TRUE(iter_a == iter_b);
    EXPECT_TRUE(*iter_a == *iter_b);
}

// When an iterator is dereferenced, can the object members be accessed?
// Test of the arrow operator (->).
TYPED_TEST(iterator_conformance_t, deref_arrow)
{
    auto_transaction_t tx;
    this->insert_records(1);

    TypeParam it = this->get_begin(it);

    EXPECT_EQ(string(it->street()), to_string(0)) << "The class member derefence operator->() does not work.";
}

// Does (void)iter++ have the same effect as (void)++iter?
TYPED_TEST(iterator_conformance_t, pre_inc_and_post_inc)
{
    auto_transaction_t tx;
    this->insert_records(3);

    TypeParam iter_a = this->get_begin(iter_a);
    TypeParam iter_b = this->get_begin(iter_b);

    (void)++iter_a;
    (void)iter_b++;

    const char* c_inc_error = "(void)++iter and (void)iter++ have different effects.";

    EXPECT_TRUE(iter_a == iter_b) << c_inc_error;
    EXPECT_STREQ(iter_a->street(), iter_b->street()) << c_inc_error;

    (void)++iter_a;
    (void)iter_b++;

    EXPECT_EQ(iter_a, iter_b) << c_inc_error;
    EXPECT_EQ(string(iter_a->street()), string(iter_b->street())) << c_inc_error;
}

// Does dereferencing and postincrementing *iter++ have the expected effects?
TYPED_TEST(iterator_conformance_t, deref_and_postinc)
{
    auto_transaction_t tx;
    this->insert_records(3);

    TypeParam iter_a = this->get_begin(iter_a);
    TypeParam iter_b = this->get_begin(iter_b);

    address_t address = *iter_b;
    ++iter_b;

    const char* c_post_inc_error = "*iter++ does not have the expected effects.";
    EXPECT_STREQ((*iter_a++).street(), address.street()) << c_post_inc_error;

    address = *iter_b;
    ++iter_b;
    EXPECT_STREQ((*iter_a++).street(), address.street()) << c_post_inc_error;

    address = *iter_b;
    ++iter_b;
    EXPECT_STREQ((*iter_a++).street(), address.street()) << c_post_inc_error;
}

// Tests for LegacyForwardIterator conformance
// ================================

// Is the iterator DefaultConstructible?
TYPED_TEST(iterator_conformance_t, default_constructible)
{
    EXPECT_TRUE(is_default_constructible<TypeParam>::value) << "The iterator is not DefaultConstructible.";
}

// Is equality and inequality defined over all iterators for the same
// underlying sequence?
TYPED_TEST(iterator_conformance_t, equality_and_inequality_in_sequence)
{
    const int c_count = 10;
    auto_transaction_t tx;
    this->insert_records(c_count);

    TypeParam iter_a = this->get_begin(iter_a);
    TypeParam iter_b = this->get_begin(iter_b);
    TypeParam iter_begin;
    TypeParam iter_end = this->get_end(iter_end);

    const char* c_equality_error = "Equality comparisons are not defined across all iterators in the same sequence.";
    const char* c_inequality_error
        = "Inequality comparisons are not defined across all iterators in the same sequence.";

    for (; iter_b != iter_end; ++iter_b)
    {
        EXPECT_TRUE(iter_a == iter_b) << c_equality_error;
        ++iter_a;
    }

    TypeParam iter = this->get_begin(iter);
    iter_begin = this->get_begin(iter_begin);

    for (; iter != iter_end; ++iter)
    {
        if (iter == iter_begin)
        {
            EXPECT_TRUE(iter != iter_end) << c_inequality_error;
        }
        else
        {
            EXPECT_TRUE(iter != iter_end) << c_inequality_error;
        }
    }
}

// Does post-incrementing the iterator have the expected effects?
TYPED_TEST(iterator_conformance_t, post_increment)
{
    auto_transaction_t tx;
    this->insert_records(3);

    TypeParam iter_a = this->get_begin(iter_a);
    TypeParam iter_b = this->get_begin(iter_b);

    EXPECT_TRUE(iter_a++ == iter_b);
    iter_b++;
    EXPECT_TRUE(iter_a++ == iter_b);
    iter_b++;
    EXPECT_TRUE(iter_a == iter_b);
}

// Can an iterator iterate over a sequence multiple times to return the same
// values at the same positions every time? This is known as the multipass
// guarantee.
TYPED_TEST(iterator_conformance_t, multipass_guarantee)
{
    const int c_count = 10;

    auto_transaction_t tx;
    this->insert_records(c_count);

    vector<address_t> sequence;

    TypeParam iter;

    int i, j;

    // Load the sequence.
    iter = this->get_begin(iter);
    for (j = 0; j < c_count; j++)
    {
        sequence.push_back(*iter++);
    }

    // Now ensure the sequence is the same over two iterations
    for (i = 0; i < 2; i++)
    {
        iter = this->get_begin(iter);
        for (j = 0; j < c_count; j++)
        {
            EXPECT_TRUE(*iter++ == sequence.at(j)) << "The iterator does not support a multipass guarantee.";
        }
    }
}

// Sanity check that our iterators can work with a std::algorithms container
// method.  Transforms list of characters into list of numbers.
TYPED_TEST(iterator_conformance_t, algorithm_test)
{
    const int c_count = 10;
    vector<int> transform_list;

    auto_transaction_t tx;
    TypeParam iter;

    this->insert_records(c_count);

    std::transform(
        this->get_begin(iter), this->get_end(iter),
        back_inserter(transform_list),
        [](const address_t& address) -> int { return atoi(address.street()); });

    EXPECT_EQ(transform_list.size(), c_count);
}
