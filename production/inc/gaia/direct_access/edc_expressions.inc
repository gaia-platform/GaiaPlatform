/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

namespace gaia
{
namespace direct_access
{

template <typename T_class, typename T_return>
T_return member_accessor_t<T_class, T_return>::operator()(const T_class& obj) const
{
    if (std::holds_alternative<member_accessor_ptr_t>(m_member_accessor))
    {
        return std::invoke(std::get<member_accessor_ptr_t>(m_member_accessor), obj);
    }
    else
    {
        return std::get<member_accessor_fn_t>(m_member_accessor)(obj);
    }
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
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator==(const std::string& value)
{
    return expression_decorator_t<T_class>(
        [value, this](const T_class& e) {
            return strcmp(m_member_accessor(e), value.c_str()) == 0;
        });
}

template <typename T_class, typename T_return>
expression_decorator_t<T_class> expression_t<T_class, T_return>::operator==(const char* value)
{
    return expression_decorator_t<T_class>(
        [value, this](const T_class& e) {
            return strcmp(m_member_accessor(e), value) == 0;
        });
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
        [value, this](const T_class& e) {
            return strcmp(m_member_accessor(e), value.c_str()) != 0;
        });
}

template <typename T_class, typename T_return>
template <typename T_predicate>
expression_decorator_t<T_class> expression_t<T_class, T_return>::contains(T_predicate predicate)
{
    return expression_decorator_t<T_class>(
        [predicate, this](const T_class& e) {
            auto container = m_member_accessor(e).where(predicate);
            return container.begin() != container.end();
        });
}

template <typename T_class, typename T_return>
template <typename T_value, typename>
expression_decorator_t<T_class> expression_t<T_class, T_return>::contains_object(const T_value& object)
{
    return expression_decorator_t<T_class>(
        [&object, this](const T_class& e) {
            auto container = m_member_accessor(e).where([&object](const T_value& c) {
                return c.gaia_id() == object.id();
            });
            return container.begin() != container.end();
        });
}

template <typename T_class, typename T_return>
expression_decorator_t<T_class> expression_t<T_class, T_return>::empty()
{
    return expression_decorator_t<T_class>(
        [this](const T_class& e) {
            auto container = m_member_accessor(e);
            return container.begin() == container.end();
        });
}

template <typename T_class, typename T_return>
expression_t<T_class, int64_t> expression_t<T_class, T_return>::count()
{
    // TODO this makes ASAN complain.
    // It seems like something within member_accessor_t goes out of scope.
    //  if (std::holds_alternative<member_accessor_ptr_t>(m_member_accessor)) <== ASAN error
    // I'm pushing the PR to get help from somebody that understand this better than I do.
    auto count_accessor = direct_access::member_accessor_t<T_class, int64_t>{
        [this](const T_class& obj) {
            auto container = m_member_accessor(obj);
            return std::distance(container.begin(), container.end());
        }};

    return expression_t<T_class, int64_t>{count_accessor};
}

} // namespace direct_access
} // namespace gaia
