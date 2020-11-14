/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// adapted from https://stackoverflow.com/a/39730680

#pragma once

#include <functional>
#include <iterator>
#include <optional>
#include <type_traits>
#include <utility>

namespace gaia
{
namespace common
{
namespace iterators
{

template <typename T_output>
class generator_iterator_t
{
    using difference_type = void;
    using value_type = T_output;
    using pointer = T_output*;
    using reference = T_output;
    using iterator_category = std::input_iterator_tag;

private:
    std::optional<T_output> m_state;
    std::function<std::optional<T_output>()> m_generator;
    std::function<bool(T_output)> m_predicate;

public:
    // Returns current state.
    T_output operator*() const
    {
        return *m_state;
    }

    // Advance to the next valid state.
    generator_iterator_t& operator++()
    {
        while ((m_state = m_generator()))
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
    friend bool operator!=(generator_iterator_t const& lhs, generator_iterator_t const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    // Returns false when the generator is exhausted (has state nullopt).
    explicit operator bool() const noexcept
    {
        return m_state.has_value();
    }

    // We implicitly construct from a std::function with the right signature.
    generator_iterator_t(
        std::function<std::optional<T_output>()> g,
        std::function<bool(T_output)> p = [](T_output) { return true; })
        : m_generator(std::move(g)), m_predicate(std::move(p))
    {
        // We need to initialize the iterator to the first valid state.
        while ((m_state = m_generator()))
        {
            if (m_predicate(*m_state))
            {
                break;
            }
        }
    }

    // Default all special member functions.
    generator_iterator_t(generator_iterator_t&&) = default;
    generator_iterator_t(generator_iterator_t const&) = default;
    generator_iterator_t& operator=(generator_iterator_t&&) = default;
    generator_iterator_t& operator=(generator_iterator_t const&) = default;
    generator_iterator_t() = default;
};

template <typename T_iter>
struct range_t
{
    T_iter b, e;
    T_iter begin() const
    {
        return b;
    }
    T_iter end() const
    {
        return e;
    }
};

template <typename T_iter>
range_t<T_iter> range(T_iter b, T_iter e)
{
    return {std::move(b), std::move(e)};
}

template <typename T_iter>
range_t<T_iter> range(T_iter b)
{
    return range(std::move(b), T_iter{});
}

template <typename T_fn>
auto range_from_generator(T_fn fn)
{
    using T_val = std::decay_t<decltype(*fn())>;
    return range(generator_iterator_t<T_val>(std::move(fn)));
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
