////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include "gaia/expressions/expression_base.hpp"

namespace gaia
{
namespace expressions
{

// Pointer to an DAC class (T_bind) member accessor method.
// An DAC accessor method returns the data stored in a certain
// table column, i.e. &employee_t::name.
//
// This accessor is specific to DAC as all DAC class methods
// are const getters that do not accept any argument.

template <typename T_bind, typename T_return>
using member_accessor_ptr_t = T_return (T_bind::*)() const;

/**
 * Access data within DAC classes. Data can be accessed via member_accessor_ptr_t
 * (eg. &employee_t::name).
 *
 * Previously member_accessor_t also allowed generic functions but that functionality
 * has been split off to transform_expression_t to provide a cleaner algebra when defining templates.
 *
 * @tparam T_bind - The DAC class type this expression binds to.
 * @tparam T_return - The type this expression evaluates to.
 *
 * This class can be specialized to provide additional functionality and run-time
 * information on specific member types. The type of accessor is determined by the catalog
 * during code-generation.
 *
 */
template <typename T_bind, typename T_return>
class member_accessor_t : public expression_t<T_bind, T_return>
{
public:
    // NOLINTNEXTLINE(google-explicit-constructor)
    member_accessor_t(member_accessor_ptr_t<T_bind, T_return> member_accessor)
        : m_member_accessor(member_accessor){};

    virtual ~member_accessor_t() = default;

    T_return operator()(const T_bind& bind) const override;
    subexpression_t<T_bind, T_return> create_subexpression() const override;

private:
    member_accessor_ptr_t<T_bind, T_return> m_member_accessor;
};

#include "member_accessor.inc"

} // namespace expressions
} // namespace gaia
