/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

template <typename T_bind, typename T_value, typename T_return>
subexpression_t<T_bind, T_return> container_accessor_t<T_bind, T_value, T_return>::create_subexpression() const
{
    return std::make_shared<container_accessor_t<T_bind, T_value, T_return>>(*this);
}

template <typename T_bind, typename T_value, typename T_return>
transform_expression_t<T_bind, bool> container_accessor_t<T_bind, T_value, T_return>::contains(
    const expression_t<T_value, bool>& predicate)
{
    auto stored_predicate = predicate.create_subexpression();

    return transform_expression_t<T_bind, bool>(
        [stored_predicate, this](const T_bind& bind) {
            auto container = (*this)(bind).where([stored_predicate](const T_value& obj) {
                return (*stored_predicate)(obj);
            });
            return container.begin() != container.end();
        });
}

template <typename T_bind, typename T_value, typename T_return>
transform_expression_t<T_bind, bool> container_accessor_t<T_bind, T_value, T_return>::contains(const T_value& value)
{
    return transform_expression_t<T_bind, bool>(
        [value, this](const T_bind& bind) {
            auto container = (*this)(bind).where(
                [value](const T_value& obj) {
                    return value == obj;
                });
            return container.begin() != container.end();
        });
}

template <typename T_bind, typename T_value, typename T_return>
transform_expression_t<T_bind, bool> container_accessor_t<T_bind, T_value, T_return>::empty()
{
    return transform_expression_t<T_bind, bool>(
        [this](const T_bind& bind) {
            auto container = (*this)(bind);
            return container.begin() == container.end();
        });
}

template <typename T_bind, typename T_value, typename T_return>
transform_expression_t<T_bind, int64_t> container_accessor_t<T_bind, T_value, T_return>::count()
{
    auto count_accessor = transform_expression_t<T_bind, int64_t>{
        [this](const T_bind& bind) {
            auto container = (*this)(bind);
            return std::distance(container.begin(), container.end());
        }};

    return transform_expression_t<T_bind, int64_t>{count_accessor};
}
