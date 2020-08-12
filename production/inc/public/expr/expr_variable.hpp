#pragma once

#include "expressions.hpp"

// Variable base type for override by codegen

namespace gaia {
namespace expr {

template <typename R, typename T_var>
class variable_expression_t : public expression_t <R> {
    public:
    R accept(interpreter_base& interp) const override{
        return accept(static_cast<interpreter<T_var>&>(interp));
    }

    virtual R accept(interpreter<T_var>& interp) const {
        return interp.eval(*this);
    }

    virtual R eval(T_var& var, const interpreter<T_var>&) const {
        return eval(var);
    };

    virtual R eval(T_var& var) const = 0;
};

}
}
