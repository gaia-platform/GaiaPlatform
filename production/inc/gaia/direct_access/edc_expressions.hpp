/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <functional>
#include <type_traits>
#include <variant>

inline static int g_count = 0;

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
 * Function that given an EDC class instance can return a value
 * from it. The advantage over member_accessor_ptr_t is that
 * a function is more flexible and can return anything.
 * This allow some neat tricks such as access to nested structure
 * within the EDC class.
 *
 * The downside is that std::function is "heavier" than a function
 * pointer such as member_accessor_ptr_t.
 */
template <typename T_class, typename T_return>
using member_accessor_fn_t = std::function<T_return(const T_class&)>;

/**
 * Predicate on EDC classes (T_class).
 */
template <typename T_class>
using edc_predicate_t = std::function<bool(const T_class&)>;

/**
 * Access data within EDC classes. Data can be accessed via member_accessor_ptr_t
 * (eg. &employee_t::name) or via a generic function.
 *
 * @tparam T_class The EDC class type
 * @tparam T_return The type returned when calling the () operator.
 */
template <typename T_class, typename T_return>
class member_accessor_t
{
public:
    // NOLINTNEXTLINE(google-explicit-constructor)
    member_accessor_t(member_accessor_ptr_t<T_class, T_return> member_accessor)
        : m_member_accessor(member_accessor){};

    // NOLINTNEXTLINE(google-explicit-constructor)
    member_accessor_t(member_accessor_fn_t<T_class, T_return> member_accessor)
        : m_member_accessor(member_accessor){};

    T_return operator()(const T_class& obj) const;

private:
    member_accessor_fn_t<T_class, T_return> m_member_accessor;
};

/**
 * Decorates a predicate allowing higher level operations
 * such as Logical Operators (||, &&, etc..).
 */
template <typename T_class>
class expression_decorator_t
{
public:
    explicit expression_decorator_t(edc_predicate_t<T_class> predicate_fn)
        : m_predicate_fn(predicate_fn){};

    bool operator()(const T_class& obj) const;

    expression_decorator_t operator||(edc_predicate_t<T_class> other_predicate);

    expression_decorator_t operator&&(edc_predicate_t<T_class> other_filter);

    expression_decorator_t operator!();

private:
    edc_predicate_t<T_class> m_predicate_fn;
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
    explicit expression_t(member_accessor_t<T_class, T_return> accessor)
        : m_member_accessor(accessor){};

    template <typename T_value>
    expression_decorator_t<T_class> operator>(T_value value);

    template <typename T_value>
    expression_decorator_t<T_class> operator>=(T_value value);

    template <typename T_value>
    expression_decorator_t<T_class> operator<(T_value value);

    template <typename T_value>
    expression_decorator_t<T_class> operator<=(T_value value);

    template <typename T_value>
    expression_decorator_t<T_class> operator==(T_value value);

    template <typename T_value>
    expression_decorator_t<T_class> operator!=(T_value value);

    // --- String specialization ---

    expression_decorator_t<T_class> operator==(const std::string& value);

    expression_decorator_t<T_class> operator==(const char* value);

    expression_decorator_t<T_class> operator!=(const std::string& value);

    expression_decorator_t<T_class> operator!=(const char* value);

    // --- Containers specializations

    template <typename T_value>
    expression_decorator_t<T_class> contains(expression_decorator_t<T_value> predicate);

    template <typename T_value, typename = std::enable_if<std::is_base_of_v<edc_base_t, T_value>>>
    expression_decorator_t<T_class> contains(const T_value& object);

    expression_decorator_t<T_class> empty();

    expression_t<T_class, int64_t> count();

private:
    member_accessor_t<T_class, T_return> m_member_accessor;
};

/*@}*/
} // namespace direct_access
/*@}*/
} // namespace gaia

// Pick up our template implementation.  These still
// need to be in the header so that template specializations
// that are declared later will pick up the definitions.
#include "edc_expressions.inc"
