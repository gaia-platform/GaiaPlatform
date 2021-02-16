/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

namespace gaia
{
namespace direct_access
{

template <typename T_class, typename T_return>
T_return member_accessor_t<T_class, T_return>::operator()(const T_class& obj)
{
    return std::invoke(m_member_accessor, obj);
}

template <typename T_class>
bool expression_decorator_t<T_class>::operator()(const T_class& obj) const
{
    return m_predicate_fn(obj);
}

template <typename T_class>
expression_decorator_t<T_class> expression_decorator_t<T_class>::operator||(edc_predicate_t other_predicate)
{
    return expression_decorator_t<T_class>(
        [other_predicate, *this](const T_class& e) {
            return m_predicate_fn(e) || other_predicate(e);
        });
}

template <typename T_class>
expression_decorator_t<T_class> expression_decorator_t<T_class>::operator&&(edc_predicate_t other_filter)
{
    return expression_decorator_t<T_class>(
        [other_filter, *this](const T_class& e) {
            return m_predicate_fn(e) && other_filter(e);
        });
}

template <typename T_class>
expression_decorator_t<T_class> expression_decorator_t<T_class>::operator!()
{
    return expression_decorator_t<T_class>(
        [*this](const T_class& e) {
            return !m_predicate_fn(e);
        });
}

template <typename T_class, typename T_return>
template <typename T_value>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator>(T_value value)
{
    return expression_decorator_t<T_class>(
        [value, this](const T_class& e) {
            return m_member_accessor(e) > value;
        });
}

template <typename T_class, typename T_return>
template <typename T_value>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator>=(T_value value)
{
    return expression_decorator_t<T_class>(
        [value, this](const T_class& e) {
            return m_member_accessor(e) >= value;
        });
}

template <typename T_class, typename T_return>
template <typename T_value>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator<(T_value value)
{
    return expression_decorator_t<T_class>(
        [value, this](const T_class& e) {
            return m_member_accessor(e) < value;
        });
}

template <typename T_class, typename T_return>
template <typename T_value>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator<=(T_value value)
{
    return expression_decorator_t<T_class>(
        [value, this](const T_class& e) {
            return m_member_accessor(e) <= value;
        });
}

template <typename T_class, typename T_return>
template <typename T_value>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator==(T_value value)
{
    return expression_decorator_t<T_class>(
        [value, this](const T_class& e) {
            return m_member_accessor(e) == value;
        });
}

template <typename T_class, typename T_return>
template <typename T_value>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator!=(T_value value)
{
    return expression_decorator_t<T_class>(
        [value, this](const T_class& e) {
            return m_member_accessor(e) != value;
        });
}

template <typename T_class, typename T_return>
expression_decorator_t<T_class> expression_t<T_class, T_return>::equals(std::string_view value, string_comparison_t casing)
{
    if (casing == string_comparison_t::case_sensitive)
    {
        return expression_decorator_t<T_class>(
            [value, this](const T_class& e) {
                return m_member_accessor(e) == value;
            });
    }

    return expression_decorator_t<T_class>(
        [value, this](const T_class& e) {
            std::string_view lh = m_member_accessor(e);

            // TODO this is probably neither the most efficient nor the most correct
            //  way (considering unicode) to do a string ignore case comparison.
            //  Good enough for now.
            return std::equal(
                lh.begin(), lh.end(), value.begin(), value.end(),
                [](char a, char b) {
                    return tolower(a) == tolower(b);
                });
        });
}

template <typename T_class, typename T_return>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator==(const std::string& value)
{
    return equals(value, string_comparison_t::case_sensitive);
}

template <typename T_class, typename T_return>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator==(const char* value)
{
    return equals(value, string_comparison_t::case_sensitive);
}

template <typename T_class, typename T_return>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator!=(const char* value)
{
    return expression_decorator_t<T_class>(
        [value, this](const T_class& e) {
            return strcmp(m_member_accessor(e), value) != 0;
        });
}

template <typename T_class, typename T_return>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator!=(const std::string& value)
{
    return expression_decorator_t<T_class>(
        [&value, this](const T_class& e) {
            return strcmp(m_member_accessor(e), value.c_str()) != 0;
        });
}

} // namespace direct_access
} // namespace gaia
