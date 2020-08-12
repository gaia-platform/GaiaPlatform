#pragma once

#include "expressions.hpp"

namespace gaia {
namespace expr {

template <typename R, typename T_parent, typename T_var, typename Expr>
class field_access_expression_t : public variable_expression_t <R, T_var> {
    public:

    R accept(interpreter<T_var>& interp) const override {
        return interp.eval(*this);
    }

    R eval(T_var& obj, const interpreter<T_var>& parent) const override {
        Expr expr;
        interpreter<T_parent> interp(&parent.get_top_level());
        interp.bind(access(obj));
        return interp.eval(expr);
    }

    R eval(T_var&) const override {
        throw std::runtime_error("should not reach");
    }

    // override me!
    virtual T_parent& access(T_var& obj) const = 0;
};

}
}
