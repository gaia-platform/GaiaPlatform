#pragma once

#include <functional>
#include <tuple>
#include <utility>
#include "expressions.hpp"

namespace gaia {
namespace expr {

namespace detail {

// for use in function arguments avoid explosion in different argument overloads...
template <typename T>
class arg_wrapper_t {
    private:
    native_expression_t<T> store;
    const expression_t <T>& expr;

    public:
    arg_wrapper_t(const expression_t<T>& expr) :
    store(0),
    expr(expr) {}

    arg_wrapper_t(const T& native) :
    store(native),
    expr(store) {}

    const expression_t<T>& get_expr() {
        return expr;
    }
};
}

template<size_t ... Ns> struct sequence {};
template<size_t ... Ns> struct gen_seq;

template <size_t I, size_t ... Ns>
struct gen_seq<I, Ns...>
{
    using type = typename gen_seq<I-1, I-1, Ns...>::type;
};

template <size_t ... Ns>
struct gen_seq<0, Ns...>
{
    using type = sequence<Ns...>;
};

template <size_t N>
using sequence_t = typename gen_seq<N>::type;

template <typename R, typename F, typename ... ArgExprs>
class function_call_expression_t : public expression_t<R> {
    private:
    friend class interpreter_base;

    std::function<F> f;
    std::tuple<ArgExprs...> args;

    R call(interpreter_base& interp) const {
        return call(interp, sequence_t<sizeof...(ArgExprs)>{});
    }

    template <size_t ... Nargs>
    R call(interpreter_base& interp, sequence<Nargs...>) const {
        return f(eval_arg<expr_type<ArgExprs>>(std::get<Nargs>(args), interp)...);
    }

    public:
    function_call_expression_t(std::function<F> f, ArgExprs&&... args) :
    f(f),
    args(std::forward<ArgExprs>(args)...) {}

    virtual R accept(interpreter_base& interp) const {
        return interp.eval(*this);
    }

    template <typename T_arg>
    T_arg eval_arg (detail::arg_wrapper_t<T_arg> wrapped, interpreter_base& interp) const {
        return wrapped.get_expr().accept(interp);
    }
};

// Usage: make_evaluable_function<int(float)> myfunc(func);
// Creates an AST stub that creates a function_call_expr to the bound function

template<typename signature>
class evaluable_function;

template <typename R, typename ...Args>
class evaluable_function<R(Args...)> {
    std::function<R(Args...)> f;

    public:
    evaluable_function (std::function<R(Args...)> f) :
        f(f) {}
    template<typename ... ArgExprs>
    function_call_expression_t<R, R(Args...), ArgExprs...> operator()(ArgExprs&&... args) {
        return function_call_expression_t<R, R(Args...), ArgExprs...>(f, std::forward<ArgExprs>(args)...);
    }
};

}
}
