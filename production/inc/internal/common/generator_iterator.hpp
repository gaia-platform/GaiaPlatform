/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// adapted from https://stackoverflow.com/a/39730680

#pragma once

#include <utility>
#include <type_traits>
#include <iterator>
#include <functional>
#include <optional>

namespace gaia {
namespace common {
namespace iterators {

template <typename Output>
class generator_iterator_t {
    using difference_type = void;
    using value_type = Output;
    using pointer = Output*;
    using reference = Output;
    using iterator_category = std::input_iterator_tag;

private:
    std::optional<Output> m_state;
    std::function<std::optional<Output>()> m_generator;
    std::function<bool(Output)> m_predicate;

public:
    // Returns current state.
    Output operator*() const {
        return *m_state;
    }

    // Advance to the next valid state.
    generator_iterator_t& operator++() {
        while ((m_state = m_generator())) {
            if (m_predicate(*m_state)) {
                break;
            }
        }
        return *this;
    }

    generator_iterator_t operator++(int) {
        auto r = *this;
        ++(*this);
        return r;
    }

    // Generator iterators are only equal if they are both in the "end" state.
    friend bool operator==(generator_iterator_t const& lhs, generator_iterator_t const& rhs) noexcept {
        if (!lhs.m_state && !rhs.m_state) {
            return true;
        }
        return false;
    }
    friend bool operator!=(generator_iterator_t const& lhs, generator_iterator_t const& rhs) noexcept {
        return !(lhs == rhs);
    }

    // Returns false when the generator is exhausted (has state nullopt).
    explicit operator bool() const noexcept {
        return m_state.has_value();
    }

    // We implicitly construct from a std::function with the right signature.
    generator_iterator_t(
        std::function<std::optional<Output>()> g,
        std::function<bool(Output)> p = [](Output) { return true; }) : m_generator(std::move(g)), m_predicate(std::move(p)) {
        // We need to initialize the iterator to the first valid state.
        while ((m_state = m_generator())) {
            if (m_predicate(*m_state)) {
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

template <typename It>
struct range_t {
    It b, e;
    It begin() const { return b; }
    It end() const { return e; }
};

template <typename It>
range_t<It> range(It b, It e) {
    return {std::move(b), std::move(e)};
}

template <typename It>
range_t<It> range(It b) {
    return range(std::move(b), It{});
}

template <typename F>
auto range_from_generator(F f) {
    using V = std::decay_t<decltype(*f())>;
    return range(generator_iterator_t<V>(std::move(f)));
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
