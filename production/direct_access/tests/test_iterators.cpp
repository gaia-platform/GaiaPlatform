/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "gtest/gtest.h"
#include "gaia_addr_book.h"
#include "db_test_base.hpp"
#include "gaia_iterators.hpp"

#include "iterator_test_helper.hpp"
#include "set_iterator_test_helper.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::addr_book;

template <typename T_iterator_test_helper>
class gaia_iterator_conformance : public db_test_base_t {
public:
    using iterator_t = typename T_iterator_test_helper::iterator_t;
    using record_t = typename T_iterator_test_helper::record_t;
    using record_list_t = typename T_iterator_test_helper::record_list_t;

protected:
    T_iterator_test_helper s_iterator_helper;
};

using iterator_suite_t = ::testing::Types<iterator_test_helper_t,
    set_iterator_test_helper_t>;
TYPED_TEST_SUITE(gaia_iterator_conformance, iterator_suite_t);

// Tests for LegacyIterator conformance
// ================================

// Is the iterator CopyConstructible?
// A test for MoveConstructible is not required because it is a prerequisite
// to be CopyConstructible.
TYPED_TEST(gaia_iterator_conformance, copy_constructible) {
    using iterator_t = typename TestFixture::iterator_t;

    EXPECT_TRUE(is_copy_constructible<iterator_t>::value)
        << "The iterator is not CopyConstructible.";
}

// Is the iterator CopyAssignable?
TYPED_TEST(gaia_iterator_conformance, copy_assignable) {
    using iterator_t = typename TestFixture::iterator_t;

    EXPECT_TRUE(is_copy_assignable<iterator_t>::value)
        << "The iterator is not CopyAssignable.";
}

// Is the iterator Destructible?
TYPED_TEST(gaia_iterator_conformance, destructible) {
    using iterator_t = typename TestFixture::iterator_t;

    EXPECT_TRUE(is_destructible<iterator_t>::value)
        << "The iterator is not Destructible.";
}

// Are iterator lvalues Swappable?
TYPED_TEST(gaia_iterator_conformance, swappable) {
    using iterator_t = typename TestFixture::iterator_t;

    EXPECT_TRUE(is_swappable<iterator_t>::value)
        << "The iterator is not Swappable as an lvalue.";
}

// Does iterator_traits<gaia_iterator_t<edc*>> have member typedefs
// value_type, difference_type, reference, pointer, and iterator_category?
TYPED_TEST(gaia_iterator_conformance, iterator_traits) {
    using iterator_t = typename TestFixture::iterator_t;

    // This test can only fail in compile time.
    typename iterator_traits<iterator_t>::value_type vt;
    typename iterator_traits<iterator_t>::difference_type dt;
    typename iterator_traits<iterator_t>::reference r = vt;
    typename iterator_traits<iterator_t>::pointer p;
    typename iterator_traits<iterator_t>::iterator_category ic;
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
TYPED_TEST(gaia_iterator_conformance, pre_incrementable) {
    using record_t = typename TestFixture::record_t;
    using iterator_t = typename TestFixture::iterator_t;

    const int c_loops = 10;
    auto_transaction_t tx;
    this->s_iterator_helper.insert_records(c_loops);

    iterator_t iter = record_t::list().begin();
    for(int i = 0; i < c_loops; i++)
    {
        ASSERT_EQ(this->s_iterator_helper.deref_iter_for_string(iter),
            this->s_iterator_helper.get_string(i))
            << "The iterator is not pre-incrementable with the "
            "expected effects.";
        ++iter;
    }

    // The declaration of end_record will fail in compile-time if the
    // pre-increment operator has the wrong return type.
    iterator_t& end_record = ++iter;
    EXPECT_EQ(end_record, record_t::list().end())
        << "The iterator is not pre-incrementable with the "
        "expected effects.";
}

// Tests for LegacyInputIterator conformance
// ================================

// Is the iterator EqualityComparable?
TYPED_TEST(gaia_iterator_conformance, equality_comparable) {
    using record_t = typename TestFixture::record_t;
    using iterator_t = typename TestFixture::iterator_t;

    auto_transaction_t tx;
    this->s_iterator_helper.insert_records(3);

    iterator_t a = record_t::list().begin();
    iterator_t b = record_t::list().begin();
    iterator_t c = record_t::list().begin();

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
    c = record_t::list().end();

    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
    EXPECT_FALSE(b == c);
    EXPECT_FALSE(a == c);
}

// Does the iterator support the not-equal (!=) operator?
TYPED_TEST(gaia_iterator_conformance, not_equal) {
    using record_t = typename TestFixture::record_t;
    using iterator_t = typename TestFixture::iterator_t;

    auto_transaction_t tx;
    this->s_iterator_helper.insert_records(3);

    iterator_t a = record_t::list().begin();
    iterator_t b = record_t::list().begin();
    ++b;
    iterator_t c = record_t::list().end();

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
TYPED_TEST(gaia_iterator_conformance, reference_convertibility) {
    using iterator_t = typename TestFixture::iterator_t;

    typedef typename iterator_traits<iterator_t>::reference from_t;
    typedef typename iterator_traits<iterator_t>::value_type to_t;

    bool convertible = is_convertible<from_t, to_t>::value;

    EXPECT_TRUE(convertible)
        << "The reference iterator trait is not convertible into the"
        " value_type iterator trait.";
}

// Are iterators dereferenceable as rvalues?
TYPED_TEST(gaia_iterator_conformance, dereferenceable_rvalue) {
    using record_t = typename TestFixture::record_t;
    using iterator_t = typename TestFixture::iterator_t;

    auto_transaction_t tx;
    this->s_iterator_helper.insert_records(1);

    iterator_t iter = record_t::list().begin();

    EXPECT_EQ(this->s_iterator_helper.deref_iter_for_string(iter),
        this->s_iterator_helper.get_string(0))
        << "The iterator is not dereferenceable as an rvalue with the"
        " expected effects.";
}

// If two rvalue iterators are equal then their dereferenced values are equal.
TYPED_TEST(gaia_iterator_conformance, dereferenceable_equality) {
    using record_t = typename TestFixture::record_t;
    using iterator_t = typename TestFixture::iterator_t;

    auto_transaction_t tx;
    this->s_iterator_helper.insert_records(1);

    iterator_t iter_a = record_t::list().begin();
    iterator_t iter_b = record_t::list().begin();

    ASSERT_TRUE(iter_a == iter_b);
    EXPECT_TRUE(*iter_a == *iter_b);
}

// When an iterator is dereferenced, can the object members be accessed?
// Test of the arrow operator (->).
TYPED_TEST(gaia_iterator_conformance, deref_arrow) {
    using record_t = typename TestFixture::record_t;
    using iterator_t = typename TestFixture::iterator_t;

    auto_transaction_t tx;
    this->s_iterator_helper.insert_records(1);

    iterator_t iter = record_t::list().begin();
    EXPECT_EQ(this->s_iterator_helper.iter_member_string(iter),
        this->s_iterator_helper.get_string(0))
        << "The class member derefence operator->() does not work.";
}

// Does (void)iter++ have the same effect as (void)++iter?
TYPED_TEST(gaia_iterator_conformance, pre_inc_and_post_inc) {
    using record_t = typename TestFixture::record_t;
    using iterator_t = typename TestFixture::iterator_t;

    auto_transaction_t tx;
    this->s_iterator_helper.insert_records(3);

    iterator_t iter_a = record_t::list().begin();
    iterator_t iter_b = record_t::list().begin();

    (void)++iter_a;
    (void)iter_b++;

    EXPECT_TRUE(iter_a == iter_b)
        << "(void)++iter and (void)iter++ have different effects.";
    EXPECT_EQ(this->s_iterator_helper.iter_member_string(iter_a),
        this->s_iterator_helper.iter_member_string(iter_b))
        << "(void)++iter and (void)iter++ have different effects.";

    (void)++iter_a;
    (void)iter_b++;

    EXPECT_EQ(iter_a, iter_b)
        << "(void)++iter and (void)iter++ have different effects.";
    EXPECT_EQ(this->s_iterator_helper.iter_member_string(iter_a),
        this->s_iterator_helper.iter_member_string(iter_b))
        << "(void)++iter and (void)iter++ have different effects.";
}

// Does derefencing and postincrementing *iter++ have the expected effects?
TYPED_TEST(gaia_iterator_conformance, deref_and_postinc) {
    using record_t = typename TestFixture::record_t;
    using iterator_t = typename TestFixture::iterator_t;

    auto_transaction_t tx;
    this->s_iterator_helper.insert_records(3);

    iterator_t iter_a = record_t::list().begin();
    iterator_t iter_b = record_t::list().begin();

    record_t record = *iter_b;
    ++iter_b;
    EXPECT_EQ(
        this->s_iterator_helper.deref_and_postinc_iter_for_string(iter_a),
        this->s_iterator_helper.get_string_from_record(record))
        << "*iter++ does not have the expected effects.";

    record = *iter_b;
    ++iter_b;
    EXPECT_EQ(
        this->s_iterator_helper.deref_and_postinc_iter_for_string(iter_a),
        this->s_iterator_helper.get_string_from_record(record))
        << "*iter++ does not have the expected effects.";

    record = *iter_b;
    ++iter_b;
    EXPECT_EQ(
        this->s_iterator_helper.deref_and_postinc_iter_for_string(iter_a),
        this->s_iterator_helper.get_string_from_record(record))
        << "*iter++ does not have the expected effects.";
}

// Tests for LegacyForwardIterator conformance
// ================================

// Is the iterator DefaultConstructible?
TYPED_TEST(gaia_iterator_conformance, default_constructible) {
    using iterator_t = typename TestFixture::iterator_t;

    EXPECT_TRUE(is_default_constructible<iterator_t>::value)
        << "The iterator is not DefaultConstructible.";
}

// Is equality and inequality defined over all iterators for the same
// underlying sequence?
TYPED_TEST(gaia_iterator_conformance, equality_and_inequality_in_sequence) {
    using record_t = typename TestFixture::record_t;
    using iterator_t = typename TestFixture::iterator_t;

    auto_transaction_t tx;
    this->s_iterator_helper.insert_records(10);

    iterator_t iter_a = record_t::list().begin();
    for (iterator_t iter_b = record_t::list().begin();
            iter_b != record_t::list().end(); ++iter_b)
    {
        ASSERT_TRUE(iter_a == iter_b)
            << "Equality comparisons are not defined across all"
            " iterators in the same sequence.";
        ++iter_a;
    }

    for (iterator_t iter = record_t::list().begin();
            iter != record_t::list().end(); ++iter)
    {
        if (iter == record_t::list().begin())
        {
            ASSERT_TRUE(iter != record_t::list().end())
                << "Inequality comparisons are not defined across all"
                " iterators in the same sequence.";
        }
        else
        {
            ASSERT_TRUE(iter != record_t::list().begin())
                << "Inequality comparisons are not defined across all"
                " iterators in the same sequence.";
        }
    }
}

// Does post-incrementing the iterator have the expected effects?
TYPED_TEST(gaia_iterator_conformance, post_increment) {
    using record_t = typename TestFixture::record_t;
    using iterator_t = typename TestFixture::iterator_t;

    auto_transaction_t tx;
    this->s_iterator_helper.insert_records(3);

    iterator_t iter_a = record_t::list().begin();
    iterator_t iter_b = record_t::list().begin();

    EXPECT_TRUE(iter_a++ == iter_b);
    iter_b++;
    EXPECT_TRUE(iter_a++ == iter_b);
    iter_b++;
    EXPECT_TRUE(iter_a == iter_b);
}

// Can an iterator iterate over a sequence multiple times to return the same
// values at the same positions every time? This is known as the multipass
// guarantee.
TYPED_TEST(gaia_iterator_conformance, multipass_guarantee) {
    using record_t = typename TestFixture::record_t;
    using iterator_t = typename TestFixture::iterator_t;
    using record_list_t = typename TestFixture::record_list_t;

    const int c_length = 10;
    const int c_passes = 10;

    auto_transaction_t tx;
    record_list_t records = this->s_iterator_helper.insert_records(c_length);

    iterator_t iter = record_t::list().begin();
    for (int i = 0; i < c_passes; i++)
    {
        for (int j = 0; j < c_length; j++)
        {
            ASSERT_TRUE(*iter == records.at(j))
                << "The iterator does not support a multipass guarantee.";
            ++iter;
        }
        iter = record_t::list().begin();
    }
}

// Iterator tests that are not directly for conformance
// ================================

// Transforms make use of iterators and are a good use case to verify.
TYPED_TEST(gaia_iterator_conformance, std_transform) {
    // TODO: write a test using std::transform.
}
