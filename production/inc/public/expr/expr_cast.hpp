#pragma once

#include "expressions.hpp"

namespace gaia{
namespace expr{

// 'cast' operator
template<typename T_cast, typename T,
        typename std::enable_if<std::is_convertible<T, T_cast>::value>::type* = nullptr>
cast_expression_t<T, T_cast> cast(const expression_t<T>& expr) {
    return cast_expression_t<T, T_cast>(expr);
}

template<typename T_from, typename T_to>
class cast_expression_t : public expression_t<T_to> {
    friend class interpreter_base;
    private:
    const expression_t<T_from>& expr;

    public:
    cast_expression_t(const expression_t<T_from>& expr) :
    expr(expr) {}

    T_to accept(interpreter_base& interpreter) const override {
        return interpreter.eval(*this);
    }
};

}
}
