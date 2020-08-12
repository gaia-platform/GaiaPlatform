#pragma once

#include "expressions.hpp"
#include "expr_op_impl.hpp"

namespace gaia {
namespace expr {

class interpreter_base {
    private:
    const interpreter_base* top_level;

    public:
    interpreter_base() : top_level(nullptr){}

    explicit interpreter_base(const interpreter_base* top_level) :
    top_level(top_level) {}

    const interpreter_base& get_top_level() const {
        return (top_level) ? *top_level : *this;
    }

    template<typename T>
    T eval (const native_expression_t<T>& expr) {
        return expr.value;
    }

    template<typename T_from, typename T_to>
    T_to eval (const cast_expression_t<T_from, T_to>& expr) {
        T_from evaluated = expr.expr.accept(*this);
        return (T_to) evaluated;
    }

    template<typename T, typename R>
    R eval(const unary_expression_t<T,R, OP_NEG>& expr) {
        T operand_eval = expr.operand.accept(*this);
        return eval_neg<T>(operand_eval);
    }

    template<typename T, typename R>
    R eval(const unary_expression_t<T,R, OP_NOT>& expr) {
        T operand_eval = expr.operand.accept(*this);
        return eval_not<T>(operand_eval);
    }

    template<typename T, typename R>
    R eval(const unary_expression_t<T,R, OP_INV>& expr) {
        T operand_eval = expr.operand.accept(*this);
        return eval_inv<T>(operand_eval);
    }

    template<typename T, typename U, typename R>
    R eval (const binary_expression_t<T, U, R, OP_EQ>& expr) {
        T op1_eval = expr.op1.accept(*this);
        U op2_eval = expr.op2.accept(*this);
        return eval_eq<T, U>(op1_eval, op2_eval);
    }

    template<typename T, typename U, typename R>
    R eval (const binary_expression_t<T, U, R, OP_NEQ>& expr) {
        T op1_eval = expr.op1.accept(*this);
        U op2_eval = expr.op2.accept(*this);
        return eval_neq<T, U>(op1_eval, op2_eval);
    }

    template<typename T, typename U, typename R>
    R eval (const binary_expression_t<T, U, R, OP_GT>& expr) {
        T op1_eval = expr.op1.accept(*this);
        U op2_eval = expr.op2.accept(*this);
        return eval_gt<T, U>(op1_eval, op2_eval);
    }

    template<typename T, typename U, typename R>
    R eval (const binary_expression_t<T, U, R, OP_GE>& expr) {
        T op1_eval = expr.op1.accept(*this);
        U op2_eval = expr.op2.accept(*this);
        return eval_ge<T, U>(op1_eval, op2_eval);
    }

    template<typename T, typename U, typename R>
    R eval (const binary_expression_t<T, U, R, OP_LT>& expr) {
        T op1_eval = expr.op1.accept(*this);
        U op2_eval = expr.op2.accept(*this);
        return eval_lt<T, U>(op1_eval, op2_eval);
    }

    template<typename T, typename U, typename R>
    R eval (const binary_expression_t<T, U, R, OP_LE>& expr) {
        T op1_eval = expr.op1.accept(*this);
        U op2_eval = expr.op2.accept(*this);
        return eval_le<T, U>(op1_eval, op2_eval);
    }

    template<typename T, typename U, typename R>
    R eval (const binary_expression_t<T, U, R, OP_AND>& expr) {
        T op1_eval = expr.op1.accept(*this);
        U op2_eval = expr.op2.accept(*this);
        return eval_and<T, U>(op1_eval, op2_eval);
    }

    template<typename T, typename U, typename R>
    R eval (const binary_expression_t<T, U, R, OP_NOR>& expr) {
        T op1_eval = expr.op1.accept(*this);
        U op2_eval = expr.op2.accept(*this);
        return eval_nor<T, U>(op1_eval, op2_eval);
    }

    template<typename T, typename U, typename R>
    R eval (const binary_expression_t<T, U, R, OP_XOR>& expr) {
        T op1_eval = expr.op1.accept(*this);
        U op2_eval = expr.op2.accept(*this);
        return eval_xor<T, U>(op1_eval, op2_eval);
    }

    template<typename T, typename U, typename R>
    R eval (const binary_expression_t<T, U, R, OP_ADD>& expr) {
        T op1_eval = expr.op1.accept(*this);
        U op2_eval = expr.op2.accept(*this);
        return eval_add<T, U>(op1_eval, op2_eval);
    }

    template<typename T, typename U, typename R>
    R eval (const binary_expression_t<T, U, R, OP_SUB>& expr) {
        T op1_eval = expr.op1.accept(*this);
        U op2_eval = expr.op2.accept(*this);
        return eval_sub<T, U>(op1_eval, op2_eval);
    }

    template<typename T, typename U, typename R>
    R eval (const binary_expression_t<T, U, R, OP_MUL>& expr) {
        T op1_eval = expr.op1.accept(*this);
        U op2_eval = expr.op2.accept(*this);
        return eval_mul<T, U>(op1_eval, op2_eval);
    }

    template<typename T, typename U, typename R>
    R eval (const binary_expression_t<T, U, R, OP_DIV>& expr) {
        T op1_eval = expr.op1.accept(*this);
        U op2_eval = expr.op2.accept(*this);
        return eval_div<T, U>(op1_eval, op2_eval);
    }

    template<typename T, typename U, typename R>
    R eval (const binary_expression_t<T, U, R, OP_MOD>& expr) {
        T op1_eval = expr.op1.accept(*this);
        U op2_eval = expr.op2.accept(*this);
        return eval_mod<T, U>(op1_eval, op2_eval);
    }

    template<typename T, typename U, typename R>
    R eval (const binary_expression_t<T, U, R, OP_BAND>& expr) {
        T op1_eval = expr.op1.accept(*this);
        U op2_eval = expr.op2.accept(*this);
        return eval_band<T, U>(op1_eval, op2_eval);
    }

    template<typename T, typename U, typename R>
    R eval (const binary_expression_t<T, U, R, OP_BOR>& expr) {
        T op1_eval = expr.op1.accept(*this);
        U op2_eval = expr.op2.accept(*this);
        return eval_bor<T, U>(op1_eval, op2_eval);
    }

    template<typename T, typename U, typename R>
    R eval (const binary_expression_t<T, U, R, OP_SHL>& expr) {
        T op1_eval = expr.op1.accept(*this);
        U op2_eval = expr.op2.accept(*this);
        return eval_shl<T, U>(op1_eval, op2_eval);
    }

    template<typename T, typename U, typename R>
    R eval (const binary_expression_t<T, U, R, OP_SHR>& expr) {
        T op1_eval = expr.op1.accept(*this);
        U op2_eval = expr.op2.accept(*this);
        return eval_shr<T, U>(op1_eval, op2_eval);
    }

    template<typename R, typename F, typename ... ArgExprs>
    R eval (const function_call_expression_t<R, F, ArgExprs...>& expr) {
        return expr.call(*this);
    }
};

// interpreter needed for binding real values to variables.

template <typename T_var>
class interpreter : public interpreter_base {
    private:
    T_var* real;

    public:
    interpreter() :
    interpreter_base(nullptr),
    real(nullptr) {}

    explicit interpreter (const interpreter_base* top_level) :
    interpreter_base(top_level),
    real(nullptr) {}

    void bind (T_var& real) {
        this->real = &real;
    }

    template <typename R>
    R eval (const expression_t<R>& expr) {
        return expr.accept(*this);
    }

    template <typename R>
    R eval (const variable_expression_t<R, T_var>& expr) {
        if (!real) {
            throw std::runtime_error("bind() not called before evaluating a variable");
        }
        return expr.eval(*real, *this);
    }
};

}
}
