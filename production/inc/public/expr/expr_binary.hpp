/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <stdexcept>

#include "expr_base.hpp"
#include "expressions.hpp"
#include "expr_op_impl.hpp"
#include "param.hpp"

// Generic binary expression.

namespace gaia {
namespace expr {

template <typename T, typename U, typename R, typename OP>
class binary_expression_t: public expression_t<R> {
    friend class interpreter_base;

    protected:
    const expression_t<T>& op1;
    const expression_t<U>& op2;

    public:
    binary_expression_t(
        const expression_t<T>& op1,
        const expression_t<U>& op2):
    op1(op1),
    op2(op2){}

    R accept(interpreter_base& interpreter) const override {
        return interpreter.eval(*this);
    }
};

// Binary expression with storage for native operand
template <typename T, typename U, typename T_stored, typename R, typename OP>
class binary_expression_native_storage_t : public binary_expression_t<T, U, R, OP> {
    private:
    native_expression_t<T_stored> stored;

    public:
    binary_expression_native_storage_t (
        paramtyp<T> lhs,
        const expression_t<U>& rhs) :
        binary_expression_t<T, U, R, OP> (stored, rhs),
        stored(lhs) {}

    binary_expression_native_storage_t (
        const expression_t<T>& lhs,
        paramtyp<U> rhs) :
        binary_expression_t<T, U, R, OP> (lhs, stored),
        stored(rhs) {}
};

}
}
