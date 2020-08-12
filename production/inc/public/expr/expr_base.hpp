/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "interpreter.hpp"

// The base expression type

namespace gaia {
namespace expr {

struct expr_base_t {};

template <typename R>
class expression_t : expr_base_t{
    public:
    typedef R value_type;
    virtual R accept(interpreter_base& e) const = 0;
};

}
}
