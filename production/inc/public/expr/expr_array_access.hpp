#pragma once

#include "expressions.hpp"

namespace gaia {
namespace expr {

template <typename R, typename T_parent>
class array_access_expression_t : public expression_t<R> {
    const expression_t<size_t>& idx_expr;

    public:
    array_access_expression_t (const expression_t<size_t>& idx_expr) :
    idx_expr(idx_expr) {}

    R accept(interpreter<T_parent>& interp) const override {
        return interp.eval(*this);
    }

    R eval(const T_parent& obj, const interpreter<T_parent>& interp) const override {
        size_t idx = interp.get_top_level().eval(idx_expr);
        return access(obj, idx);
    }

    R eval(const T_parent&) const override {
        throw std::runtime_error("should not reach");
    }

    // override me!
    virtual R access(const T_parent& obj, size_t idx) const = 0;
};

template <typename R, typename T_parent>
class array_access_expression_fixed_idx_t : array_access_expression_t<R, T_parent> {
    native_expression_t<size_t> native_idx;

    public:
    array_access_expression_fixed_idx_t(size_t idx) :
    array_access_expression_t<R, T_parent>(native_idx),
    native_idx(idx) {}
};

// not an expression but an AST builder stub
template <typename R, typename T_parent>
class array_ast_stub {
    array_access_expression_t<R&, T_parent> operator[] (const expression_t<size_t>& idx_expr) {
        return array_access_expression_t<R&, T_parent>(idx_expr);
    }
    array_access_expression_t<R&, T_parent> operator[] (size_t idx) {
        return array_access_expression_fixed_idx_t<R&, T_parent>(idx);
    }
};

}
}
