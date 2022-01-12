/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once
#include <functional>
#include <memory>

namespace gaia
{
namespace expressions
{
struct expression_base_t
{
};

/*
 * Base expression type. All expressions are derived from this type.
 *
 * T_bind - context this expression binds to (e.g DAC bindect, projected tuple).
 * T_return - the C++ type that this expression resolves to.
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
    virtual T_return operator()(const T_bind&) const = 0;

    /*
     * Create a subexpression. This is to provide storage + polymorphism for expression types.
     * Unfortunately std::function/functor requires all types to be copyable, so we are forced
     * to use std::shared_pointer. That is unless/until we add zero-copy specific interfaces as
     * an optimization in DAC.
     * */
    virtual std::shared_ptr<expression_t<T_bind, T_return>> create_subexpression() const = 0;
};

template <typename T_bind, typename T_return>
using subexpression_t = std::shared_ptr<expression_t<T_bind, T_return>>;

} // namespace expressions
} // namespace gaia
