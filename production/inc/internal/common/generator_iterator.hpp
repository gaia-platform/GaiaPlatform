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
class generator_iterator {
    using difference_type = void;
    using value_type = Output;
    using pointer = Output *;
    using reference = Output;
    using iterator_category = std::input_iterator_tag;

  private:
    std::optional<Output> state;
    std::function<std::optional<Output>()> generator;
    std::function<bool(Output)> predicate;

  public:
    // Returns current state.
    Output operator*() const {
        return *state;
    }

    // Advance to the next valid state.
    generator_iterator &operator++() {
        while ((state = generator())) {
            if (predicate(*state)) {
                break;
            }
        }
        return *this;
    }

    generator_iterator operator++(int) {
        auto r = *this;
        ++(*this);
        return r;
    }

    // Generator iterators are only equal if they are both in the "end" state.
    friend bool operator==(generator_iterator const &lhs, generator_iterator const &rhs) noexcept {
        if (!lhs.state && !rhs.state) {
            return true;
        }
        return false;
    }
    friend bool operator!=(generator_iterator const &lhs, generator_iterator const &rhs) noexcept {
        return !(lhs == rhs);
    }

    // Returns false when the generator is exhausted (has state nullopt).
    explicit operator bool() const noexcept {
        return state.has_value();
    }

    // We implicitly construct from a std::function with the right signature.
    generator_iterator(
        std::function<std::optional<Output>()> g,
        std::function<bool(Output)> p = [](Output) { return true; }) : generator(std::move(g)), predicate(std::move(p)) {
        // We need to initialize the iterator to the first valid state.
        while ((state = generator())) {
            if (predicate(*state)) {
                break;
            }
        }
    }

    // Default all special member functions.
    generator_iterator(generator_iterator &&) = default;
    generator_iterator(generator_iterator const &) = default;
    generator_iterator &operator=(generator_iterator &&) = default;
    generator_iterator &operator=(generator_iterator const &) = default;
    generator_iterator() = default;
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
    return range(generator_iterator<V>(std::move(f)));
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
