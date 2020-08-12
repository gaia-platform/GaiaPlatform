/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "expr_base.hpp"

namespace gaia{
namespace expr{

// Wrapper for a native value.
template <typename T>
class native_expression_t : public expression_t <T> {
    friend class interpreter_base;

    private:
    T value;

    public:
    native_expression_t(T value) : value(value) {}
    native_expression_t(const native_expression_t<T>& other) = default;

    T accept(interpreter_base& interpreter) const override{
        return interpreter.eval(*this);
    }
};

}
}
