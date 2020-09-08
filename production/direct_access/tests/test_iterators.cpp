/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cstdlib>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "gtest/gtest.h"
#include "gaia_addr_book.h"
#include "db_test_base.hpp"
#include "gaia_iterators.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::addr_book;

class gaia_iterator_test : public db_test_base_t {
public:
    typedef employee_t record_t;
    typedef gaia_iterator_t<record_t> iterator_t;
    typedef vector<record_t> record_vec_t;

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

    string get_index_string(const int index)
    {
        return to_string(index);
    }

    record_vec_t insert_records(const int amount)
    {
        auto emp_writer = employee_writer();
        record_vec_t vec;

        for (int i = 0; i < amount; i++)
        {
            emp_writer.name_first = get_index_string(i);
            vec.push_back(record_t::get(
                emp_writer.insert_row()));
        }
        return vec;
    }

    string deref_iter_for_string(iterator_t& iter)
    {
        string str((*iter).name_first());
        return str;
    }

    string deref_and_postinc_iter_for_string(iterator_t& iter)
    {
        string str((*iter++).name_first());
        return str;
    }

    string iter_member_string(iterator_t& iter)
    {
        string str(iter->name_first());
        return str;
    }

    string get_string_from_record(record_t& record)
    {
        string str(record.name_first());
        return str;
    }
};

// Tests for LegacyIterator conformance
// ================================

// Is the iterator CopyConstructible?
// A test for MoveConstructible is not required because it is a prerequisite
// to be CopyConstructible.
TEST_F(gaia_iterator_test, copy_constructible) {
    EXPECT_TRUE(is_copy_constructible<iterator_t>::value)
        << "The iterator is not CopyConstructible.";
}

// Is the iterator CopyAssignable?
TEST_F(gaia_iterator_test, copy_assignable) {
    EXPECT_TRUE(is_copy_assignable<iterator_t>::value)
        << "The iterator is not CopyAssignable.";
}

// Is the iterator Destructible?
TEST_F(gaia_iterator_test, destructible) {
    EXPECT_TRUE(is_destructible<iterator_t>::value)
        << "The iterator is not Destructible.";
}

// Are iterator lvalues Swappable?
TEST_F(gaia_iterator_test, swappable) {
    EXPECT_TRUE(is_swappable<iterator_t>::value)
        << "The iterator is not Swappable as an lvalue.";
}

// Does iterator_traits<gaia_iterator_t<edc*>> have member typedefs
// value_type, difference_type, reference, pointer, and iterator_category?
TEST_F(gaia_iterator_test, iterator_traits) {
    // This test can only fail in compile time.
    iterator_traits<iterator_t>::value_type vt;
    iterator_traits<iterator_t>::difference_type dt;
    iterator_traits<iterator_t>::reference r = vt;
    iterator_traits<iterator_t>::pointer p;
    iterator_traits<iterator_t>::iterator_category ic;
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
    const int LOOPS = 10;
    auto_transaction_t tx;
    insert_records(LOOPS);

    iterator_t iter = record_t::list().begin();
    for(int i = 0; i < LOOPS; i++)
    {
        ASSERT_EQ(deref_iter_for_string(iter), get_index_string(i))
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
TEST_F(gaia_iterator_test, equality_comparable) {
    auto_transaction_t tx;
    insert_records(3);

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
TEST_F(gaia_iterator_test, not_equal) {
    auto_transaction_t tx;
    insert_records(3);

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
TEST_F(gaia_iterator_test, reference_convertibility) {
    typedef iterator_traits<iterator_t>::reference from_t;
    typedef iterator_traits<iterator_t>::value_type to_t;

    bool convertible = is_convertible<from_t, to_t>::value;

    EXPECT_TRUE(convertible)
        << "The reference iterator trait is not convertible into the "
        "value_type iterator trait.";
}

// Are iterators dereferenceable as rvalues?
TEST_F(gaia_iterator_test, dereferenceable_rvalue) {
    auto_transaction_t tx;
    insert_records(1);

    iterator_t iter = record_t::list().begin();

    EXPECT_EQ(deref_iter_for_string(iter), get_index_string(0))
        << "The iterator is not dereferenceable as an rvalue with the "
        "expected effects.";
}

// If two rvalue iterators are equal then their dereferenced values are equal.
TEST_F(gaia_iterator_test, dereferenceable_equality) {
    auto_transaction_t tx;
    insert_records(1);

    iterator_t iter_a = record_t::list().begin();
    iterator_t iter_b = record_t::list().begin();

    ASSERT_TRUE(iter_a == iter_b);
    EXPECT_TRUE(*iter_a == *iter_b);
}

// When an iterator is dereferenced, can the object members be accessed?
// Test of the arrow operator (->).
TEST_F(gaia_iterator_test, deref_arrow) {
    auto_transaction_t tx;
    insert_records(1);

    iterator_t iter = record_t::list().begin();
    EXPECT_EQ(iter_member_string(iter), get_index_string(0))
        << "The class member derefence operator->() does not work.";
}

// Does (void)iter++ have the same effect as (void)++iter?
TEST_F(gaia_iterator_test, pre_inc_and_post_inc) {
    auto_transaction_t tx;
    insert_records(3);

    iterator_t iter_a = record_t::list().begin();
    iterator_t iter_b = record_t::list().begin();

    (void)++iter_a;
    (void)iter_b++;

    EXPECT_TRUE(iter_a == iter_b)
        << "(void)++iter and (void)iter++ have different effects.";
    EXPECT_EQ(iter_member_string(iter_a), iter_member_string(iter_b))
        << "(void)++iter and (void)iter++ have different effects.";

    (void)++iter_a;
    (void)iter_b++;

    EXPECT_EQ(iter_a, iter_b)
        << "(void)++iter and (void)iter++ have different effects.";
    EXPECT_EQ(iter_member_string(iter_a), iter_member_string(iter_b))
        << "(void)++iter and (void)iter++ have different effects.";
}

// Does derefencing and postincrementing *iter++ have the expected effects?
TEST_F(gaia_iterator_test, deref_and_postinc) {
    auto_transaction_t tx;
    insert_records(3);

    iterator_t iter_a = record_t::list().begin();
    iterator_t iter_b = record_t::list().begin();

    record_t record = *iter_b;
    ++iter_b;
    EXPECT_EQ(deref_and_postinc_iter_for_string(iter_a),
        get_string_from_record(record))
        << "*iter++ does not have the expected effects.";

    record = *iter_b;
    ++iter_b;
    EXPECT_EQ(deref_and_postinc_iter_for_string(iter_a),
        get_string_from_record(record))
        << "*iter++ does not have the expected effects.";

    record = *iter_b;
    ++iter_b;
    EXPECT_EQ(deref_and_postinc_iter_for_string(iter_a),
        get_string_from_record(record))
        << "*iter++ does not have the expected effects.";
}

// Tests for LegacyForwardIterator conformance
// ================================

// Is the iterator DefaultConstructible?
TEST_F(gaia_iterator_test, default_constructible) {
    EXPECT_TRUE(is_default_constructible<iterator_t>::value)
        << "The iterator is not DefaultConstructible.";
}

// Is equality and inequality defined over all iterators for the same
// underlying sequence?
TEST_F(gaia_iterator_test, equality_and_inequality_in_sequence) {
    auto_transaction_t tx;
    insert_records(10);

    iterator_t iter_a = record_t::list().begin();
    for (iterator_t iter_b = record_t::list().begin();
            iter_b != record_t::list().end(); ++iter_b)
    {
        ASSERT_TRUE(iter_a == iter_b)
            << "Equality comparisons are not defined across all iterators in the same sequence.";
        ++iter_a;
    }

    for (iterator_t iter = record_t::list().begin();
            iter != record_t::list().end(); ++iter)
    {
        if (iter == record_t::list().begin())
        {
            ASSERT_TRUE(iter != record_t::list().end())
                << "Inequality comparisons are not defined across all iterators in the same sequence.";
        }
        else
        {
            ASSERT_TRUE(iter != record_t::list().begin())
                << "Inequality comparisons are not defined across all iterators in the same sequence.";
        }
    }
}

// Does post-incrementing the iterator have the expected effects?
TEST_F(gaia_iterator_test, post_increment) {
    auto_transaction_t tx;
    insert_records(3);

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
TEST_F(gaia_iterator_test, multipass_guarantee) {
    const int LENGTH = 10;
    const int PASSES = 10;

    auto_transaction_t tx;
    record_vec_t records = insert_records(LENGTH);

    iterator_t iter = record_t::list().begin();
    for (int i = 0; i < PASSES; i++)
    {
        for (int j = 0; j < LENGTH; j++)
        {
            ASSERT_TRUE(*iter == records.at(j))
                << "The iterator does not support a multipass guarantee.";
            ++iter;
        }
        iter = record_t::list().begin();
    }
}
