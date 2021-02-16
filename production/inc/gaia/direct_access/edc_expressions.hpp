/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <functional>

#include "gaia_internal/common/timer.hpp"

namespace gaia
{

/**
 * \addtogroup Gaia
 * @{
 */

namespace direct_access
{

/**
 * \addtogroup Direct
 * @{
 *
 * API to build expressions to perform operations on EDC classes,
 * such as filtering. An example of expression is: name == "Jhon".
 */

/**
 * Pointer to an EDC class (T_class) member accessor method.
 * An EDC accessor method returns the data stored in a certain
 * table column, i.e. &employee_t::name.
 *
 * This accessor is specific to EDC as all EDC class methods
 * are const getters that do not accept any argument.
 */
template <typename T_class, typename T_return>
using member_accessor_ptr_t = T_return (T_class::*)() const;

/**
 * Predicate on EDC classes (T_class).
 */
template <typename T_class>
using edc_predicate_t = std::function<bool(const T_class&)>;

template <typename T_class, typename T_return>
class member_accessor_t
{
public:
    using member_accessor_ptr_t = member_accessor_ptr_t<T_class, T_return>;

    // NOLINTNEXTLINE(google-explicit-constructor)
    member_accessor_t(
        member_accessor_ptr_t member_accessor)
        : m_member_accessor(member_accessor){};

    T_return operator()(const T_class& obj);

private:
    member_accessor_ptr_t m_member_accessor;
};

/**
 * Used on string comparisons to determine the casing
 * of the operations.
 */
enum class string_comparison_t
{
    case_insensitive,
    case_sensitive
};

/**
 * Decorates a predicate allowing higher level operations
 * such as Logical Operators (||, &&, etc..).
 */
template <typename T_class>
class expression_decorator_t
{
public:
    using edc_predicate_t = edc_predicate_t<T_class>;

    // The constructor is templated because often 'predicate_fn' is a lambda
    // and lambda is not convertible to edc_predicate_t (in this context).
    explicit expression_decorator_t(edc_predicate_t predicate_fn)
        : m_predicate_fn(predicate_fn){};

    bool operator()(const T_class& obj) const;

    expression_decorator_t operator||(edc_predicate_t other_predicate);

    expression_decorator_t operator&&(edc_predicate_t other_filter);

    expression_decorator_t operator!();

private:
    edc_predicate_t m_predicate_fn;
};

/**
 * Represent an expression for EDC classes. At the moment, only boolean expressions
 * (predicates) are supported.
 *
 * The predicates test the value extracted from the EDC instance, via a member_accessor_t,
 * against an arbitrary value using one of the available operations, i.e. <, >, ==, etc..
 *
 * For instance:
 *
 *  auto simone = ...// get employee whose name is "Simone"
 *  auto name = expression_t<employee_t, const char *>(&employee_t::name);
 *
 *  auto name_predicate = name == "Simone";
 *  cout << name_predicate(simone) << endl; // -> 1
 *
 *  auto name_predicate2 = name == "George";
 *  cout << name_predicate2(simone) << endl; // -> 0
 *
 * @tparam T_class The EDC class the predicate will act on.
 * @tparam T_return The return type of the EDC class member.
 */
template <typename T_class, typename T_return>
class expression_t
{
public:
    using member_accessor_t = member_accessor_t<T_class, T_return>;
    using predicate_decorator_t = expression_decorator_t<T_class>;

    explicit expression_t(member_accessor_t accessor)
        : m_member_accessor(accessor){};

    template <typename T_value>
    predicate_decorator_t operator>(T_value value);

    template <typename T_value>
    predicate_decorator_t operator>=(T_value value);

    template <typename T_value>
    predicate_decorator_t operator<(T_value value);

    template <typename T_value>
    predicate_decorator_t operator<=(T_value value);

    template <typename T_value>
    predicate_decorator_t operator==(T_value value);

    template <typename T_value>
    predicate_decorator_t operator!=(T_value value);

    // --- String specialization ---

    predicate_decorator_t equals(std::string_view value, string_comparison_t casing = string_comparison_t::case_sensitive);

    predicate_decorator_t operator==(const std::string& value);

    predicate_decorator_t operator==(const char* value);

    predicate_decorator_t operator!=(const std::string& value);

    predicate_decorator_t operator!=(const char* value);

private:
    member_accessor_t m_member_accessor;
};

/*@}*/
} // namespace direct_access
/*@}*/
} // namespace gaia

// Pick up our template implementation.  These still
// need to be in the header so that template specializations
// that are declared later will pick up the definitions.
#include "edc_expressions.inc"
