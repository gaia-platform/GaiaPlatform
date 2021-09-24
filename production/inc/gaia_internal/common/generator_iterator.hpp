/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// adapted from https://stackoverflow.com/a/39730680

#pragma once

#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace gaia
{
namespace common
{
namespace iterators
{

// Generator_t class for inheritance for "movable semantics" in lambda.
template <typename T_output>
class generator_t
{
public:
    generator_t()
        : m_function([]() { return std::nullopt; })
    {
    }

    explicit generator_t(std::function<std::optional<T_output>()> generator_fn)
        : m_function(std::move(generator_fn))
    {
    }

    virtual std::optional<T_output> operator()()
    {
        return m_function();
    }

    virtual ~generator_t() = default;

    // Generator lifecycle function.
    virtual void cleanup(){};

private:
    std::function<std::optional<T_output>()> m_function;
};

template <typename T_output>
class generator_iterator_t
{
    using difference_type = void;
    using value_type = T_output;
    using pointer = T_output*;
    using reference = T_output;
    using iterator_category = std::input_iterator_tag;

public:
    // Default all special member functions.
    generator_iterator_t(generator_iterator_t&&) noexcept = default;
    generator_iterator_t(generator_iterator_t const&) = default;
    generator_iterator_t& operator=(generator_iterator_t&&) = default;
    generator_iterator_t& operator=(generator_iterator_t const&) = default;
    generator_iterator_t() = default;

    // We implicitly construct from a std::function with the right signature.
    explicit generator_iterator_t(
        std::function<std::optional<T_output>()> generator,
        std::function<bool(T_output)> predicate = [](T_output) { return true; })
        : m_generator(std::make_shared<generator_t<T_output>>(generator)), m_predicate(std::move(predicate))
    {
        init_generator();
    }

    explicit generator_iterator_t(
        std::shared_ptr<generator_t<T_output>> generator,
        std::function<bool(T_output)> predicate = [](T_output) { return true; })
        : m_generator(std::move(generator)), m_predicate(std::move(predicate))
    {
        init_generator();
    }

    ~generator_iterator_t()
    {
        if (m_generator)
        {
            m_generator->cleanup();
        }
    }

    // const versions of operators.
    const T_output& operator*() const
    {
        // If we de-reference m_state via *m_state, we can run into undefined behavior
        // if m_state does not contain a value, so we'll use the value() method instead,
        // which throws an exception in that case.
        return m_state.value();
    }

    const T_output* operator->() const
    {
        if (m_state.has_value())
        {
            return &m_state.value();
        }
        else
        {
            return nullptr;
        }
    }

    // Returns current state.
    T_output& operator*()
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        return const_cast<T_output&>(const_cast<const generator_iterator_t*>(this)->operator*());
    }

    T_output* operator->()
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        return const_cast<T_output*>(const_cast<const generator_iterator_t*>(this)->operator->());
    }

    // Advance to the next valid state.
    generator_iterator_t& operator++()
    {
        while ((m_state = (*m_generator)()))
        {
            if (m_predicate(*m_state))
            {
                break;
            }
        }
        return *this;
    }

    generator_iterator_t operator++(int)
    {
        auto r = *this;
        ++(*this);
        return r;
    }

    // Generator iterators are only equal if they are both in the "end" state.
    friend bool operator==(generator_iterator_t const& lhs, generator_iterator_t const& rhs) noexcept
    {
        if (!lhs.m_state && !rhs.m_state)
        {
            return true;
        }
        return false;
    }

    friend bool operator==(generator_iterator_t const& lhs, std::nullopt_t const&) noexcept
    {
        return !lhs.m_state.has_value();
    }

    friend bool operator!=(generator_iterator_t const& lhs, generator_iterator_t const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    friend bool operator!=(generator_iterator_t const& lhs, std::nullopt_t const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    // Returns false when the generator is exhausted (has state nullopt).
    explicit operator bool() const noexcept
    {
        return m_state.has_value();
    }

private:
    void init_generator()
    {
        // We need to initialize the iterator to the first valid state.
        while ((m_state = (*m_generator)()))
        {
            if (m_predicate(*m_state))
            {
                break;
            }
        }
    }

    std::optional<T_output> m_state;
    // We need to use shared_ptr here to allow generator_iterator_t to be copyable.
    // Non-copyable iterators cannot be used in range-based for loops.
    std::shared_ptr<generator_t<T_output>> m_generator;
    std::function<bool(T_output)> m_predicate;
};

template <typename T_output>
struct generator_range_t
{
    generator_iterator_t<T_output> begin_it;

    const generator_iterator_t<T_output>& begin() const
    {
        return begin_it;
    }

    const std::nullopt_t end() const
    {
        return std::nullopt;
    }
};

template <typename T_output>
auto range_from_generator_iterator(generator_iterator_t<T_output>&& generator)
{
    return generator_range_t<T_output>{std::move(generator)};
}

template <typename T_fn>
auto range_from_generator(T_fn&& fn)
{
    using T_val = std::decay_t<decltype(*fn())>;
    return range_from_generator_iterator<T_val>(generator_iterator_t<T_val>(std::move(fn)));
}

// Usage:
//
// int main() {
//     auto generator_from_1_to_10 = [i = 0]() mutable -> std::optional<int>
//     {
//         auto r = ++i;
//         if (r > 10) return {};
//         return r;
//     };
//     for (int i : range_from_generator(generator_from_1_to_10)) {
//         std::cout << i << '\n';
//     }
// }

} // namespace iterators
} // namespace common
} // namespace gaia
