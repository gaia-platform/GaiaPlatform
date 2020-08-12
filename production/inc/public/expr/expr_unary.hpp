#pragma once

#include <stdexcept>

#include "expr_base.hpp"
#include "expr_op_impl.hpp"

// Unary expression

namespace gaia {
namespace expr {

template <typename T, typename R, typename OP>
class unary_expression_t : public expression_t<R> {
    friend class interpreter_base;

    private:
    const expression_t<T>& operand;

    public:
    unary_expression_t(const expression_t<T>& operand) :
        operand(operand) {}

    R accept(interpreter_base& interpreter) const override{
        return interpreter.eval(*this);
    }
};

}
}
