/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

namespace gaia
{
namespace direct_access
{

template <typename T_class, typename T_return>
T_return member_accessor_t<T_class, T_return>::operator()(const T_class& obj) const
{
    return m_member_accessor(obj);
}

template <typename T_class>
bool expression_decorator_t<T_class>::operator()(const T_class& obj) const
{
    return m_predicate_fn(obj);
}

template <typename T_class>
expression_decorator_t<T_class> expression_decorator_t<T_class>::operator||(edc_predicate_t<T_class> other_predicate)
{
    return expression_decorator_t<T_class>(
        [other_predicate = other_predicate, predicate_fn = m_predicate_fn](const T_class& e) {
            return predicate_fn(e) || other_predicate(e);
        });
}

template <typename T_class>
expression_decorator_t<T_class> expression_decorator_t<T_class>::operator&&(edc_predicate_t<T_class> other_predicate)
{
    return expression_decorator_t<T_class>(
        [other_predicate = other_predicate, predicate_fn = m_predicate_fn](const T_class& e) {
            return predicate_fn(e) && other_predicate(e);
        });
}

template <typename T_class>
expression_decorator_t<T_class> expression_decorator_t<T_class>::operator!()
{
    return expression_decorator_t<T_class>(
        [predicate_fn = m_predicate_fn](const T_class& e) {
            return !predicate_fn(e);
        });
}

template <typename T_class, typename T_return>
template <typename T_value>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator>(T_value value)
{
    return expression_decorator_t<T_class>(
        [value = value, member_accessor = m_member_accessor](const T_class& e) {
            return member_accessor(e) > value;
        });
}

template <typename T_class, typename T_return>
template <typename T_value>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator>=(T_value value)
{
    return expression_decorator_t<T_class>(
        [value = value, member_accessor = m_member_accessor](const T_class& e) {
            return member_accessor(e) >= value;
        });
}

template <typename T_class, typename T_return>
template <typename T_value>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator<(T_value value)
{
    return expression_decorator_t<T_class>(
        [value = value, member_accessor = m_member_accessor](const T_class& e) {
            return member_accessor(e) < value;
        });
}

template <typename T_class, typename T_return>
template <typename T_value>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator<=(T_value value)
{
    return expression_decorator_t<T_class>(
        [value = value, member_accessor = m_member_accessor](const T_class& e) {
            return member_accessor(e) <= value;
        });
}

template <typename T_class, typename T_return>
template <typename T_value>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator==(T_value value)
{
    return expression_decorator_t<T_class>(
        [value = value, member_accessor = m_member_accessor](const T_class& e) {
            return member_accessor(e) == value;
        });
}

template <typename T_class, typename T_return>
template <typename T_value>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator!=(T_value value)
{
    return expression_decorator_t<T_class>(
        [value = value, member_accessor = m_member_accessor](const T_class& e) {
            return member_accessor(e) != value;
        });
}

template <typename T_class, typename T_return>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator==(const std::string& value)
{
    return expression_decorator_t<T_class>(
        [value = value, member_accessor = m_member_accessor](const T_class& e) {
            return strcmp(member_accessor(e), value.c_str()) == 0;
        });
}

template <typename T_class, typename T_return>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator==(const char* value)
{
    return expression_decorator_t<T_class>(
        [value = value, member_accessor = m_member_accessor](const T_class& e) {
            return strcmp(member_accessor(e), value) == 0;
        });
}

template <typename T_class, typename T_return>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator!=(const char* value)
{
    return expression_decorator_t<T_class>(
        [value = value, member_accessor = m_member_accessor](const T_class& e) {
            return strcmp(member_accessor(e), value) != 0;
        });
}

template <typename T_class, typename T_return>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator!=(const std::string& value)
{
    return expression_decorator_t<T_class>(
        [value = value, member_accessor = m_member_accessor](const T_class& e) {
            return strcmp(member_accessor(e), value.c_str()) != 0;
        });
}

template <typename T_class, typename T_return>
template <typename T_value>
expression_decorator_t<T_class> expression_t<T_class, T_return>::contains(
    expression_decorator_t<T_value> predicate)
{
    return expression_decorator_t<T_class>(
        [predicate, member_accessor = m_member_accessor](const T_class& e) {
            auto container = member_accessor(e).where(predicate);
            return container.begin() != container.end();
        });
}

template <typename T_class, typename T_return>
template <typename T_value, typename>
expression_decorator_t<T_class> expression_t<T_class, T_return>::contains(const T_value& object)
{
    return expression_decorator_t<T_class>(
        [&object, member_accessor = m_member_accessor](const T_class& e) {
            auto container = member_accessor(e).where(
                [&object](const T_value& c) {
                    return c == object;
                });
            return container.begin() != container.end();
        });
}

template <typename T_class, typename T_return>
expression_decorator_t<T_class> expression_t<T_class, T_return>::empty()
{
    return expression_decorator_t<T_class>(
        [member_accessor = m_member_accessor](const T_class& e) {
            auto container = member_accessor(e);
            return container.begin() == container.end();
        });
}

template <typename T_class, typename T_return>
expression_t<T_class, int64_t> expression_t<T_class, T_return>::count()
{
    auto count_accessor = direct_access::member_accessor_t<T_class, int64_t>{
        [member_accessor = m_member_accessor](const T_class& obj) {
            auto container = member_accessor(obj);
            return std::distance(container.begin(), container.end());
        }};

    return expression_t<T_class, int64_t>{count_accessor};
}

} // namespace direct_access
} // namespace gaia
