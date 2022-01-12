/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "expression_base.hpp"
namespace gaia
{
/**
 * \addtogroup gaia
 * @{
 */
namespace expressions
{
/**
 * Function that given an DAC class instance can return a value
 * from it. The advantage over member_accessor_ptr_t is that
 * a function is more flexible and can return anything.
 * This allow some neat tricks such as access to nested structure
 * within the DAC class.
 */
template <typename T_bind, typename T_return>
using transform_fn_t = std::function<T_return(const T_bind&)>;

template <typename T_bind, typename T_return>
class transform_expression_t : public expression_t<T_bind, T_return>
{
public:
    // NOLINTNEXTLINE(google-explicit-constructor)
    transform_expression_t(transform_fn_t<T_bind, T_return> transform_fn)
        : m_transform_fn(transform_fn){};
    virtual ~transform_expression_t() = default;

    T_return operator()(const T_bind& bind) const override;
    subexpression_t<T_bind, T_return> create_subexpression() const override;

private:
    transform_fn_t<T_bind, T_return> m_transform_fn;
};

#include "transform_expression.inc"

} // namespace expressions
} // namespace gaia
