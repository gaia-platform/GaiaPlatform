/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <functional>
#include <memory>

namespace gaia
{
/**
 * @addtogroup gaia
 * @{
 */
namespace expressions
{
/**
 * @addtogroup expressions
 * @{
 */

// Empty non-templated base class for template is_expression<> macro to pick up.
// This is because the templated classes are considered separate classes in C++,
// Adding a common empty struct allows for an unambiguous way for the macro to be defined.
struct expression_base_t
{
};

/**
 * Expression type. All expressions are derived from this type.
 *
 * @tparam T_bind - context this expression binds to (e.g DAC object, projected tuple).
 * @tparam T_return - the C++ type that this expression resolves to.
 *
 * Expressions are also C++ functors over the context they bind to,
 * i.e. they have a operator(const T_bind&) interface. This allows expressions
 * to be evaluated at run-time.
 */

template <typename T_bind, typename T_return>
class expression_t : public expression_base_t
{
public:
    virtual ~expression_t() = default;
    virtual T_return operator()(const T_bind& bind) const = 0;

    // Create a subexpression. This is to provide storage + polymorphism for expression types.
    // Unfortunately std::function/functor requires all types to be copyable, so we are forced
    // to use std::shared_pointer. That is unless/until we add zero-copy specific interfaces as
    // an optimization in DAC.
    virtual std::shared_ptr<expression_t<T_bind, T_return>> create_subexpression() const = 0;
};

template <typename T_bind, typename T_return>
using subexpression_t = std::shared_ptr<expression_t<T_bind, T_return>>;

/*@}*/
} // namespace expressions
/*@}*/
} // namespace gaia
