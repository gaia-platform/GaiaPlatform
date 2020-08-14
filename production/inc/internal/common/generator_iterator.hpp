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

template <typename T>
struct generator_iterator {
    using difference_type = void;
    using value_type = T;
    using pointer = T *;
    using reference = T;
    using iterator_category = std::input_iterator_tag;
    std::optional<T> state;
    std::function<std::optional<T>()> generator;
    // we store the current element in "state" if we have one:
    T operator*() const {
        return *state;
    }
    // to advance, we invoke our generator.  If it returns a nullopt
    // we have reached the end:
    generator_iterator &operator++() {
        state = generator();
        return *this;
    }
    generator_iterator operator++(int) {
        auto r = *this;
        ++(*this);
        return r;
    }
    // generator iterators are only equal if they are both in the "end" state:
    friend bool operator==(generator_iterator const &lhs, generator_iterator const &rhs) noexcept {
        if (!lhs.state && !rhs.state) {
            return true;
        }
        return false;
    }
    friend bool operator!=(generator_iterator const &lhs, generator_iterator const &rhs) noexcept {
        return !(lhs == rhs);
    }
    // returns false when the generator is exhausted (returns nullopt):
    explicit operator bool() const noexcept {
        return state.has_value();
    }
    // We implicitly construct from a std::function with the right signature:
    generator_iterator(std::function<std::optional<T>()> f) : generator(std::move(f)) {
        if (generator) {
            state = generator();
        }
    }
    // default all special member functions:
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
auto generator(F f) {
    using V = std::decay_t<decltype(*f())>;
    return range(generator_iterator<V>(std::move(f)));
}

// Usage:
//
// int main() {
//     auto from_1_to_10 = [i = 0]() mutable -> std::optional<int>
//     {
//         auto r = ++i;
//         if (r > 10) return {};
//         return r;
//     };
//     for (int i : generator(from_1_to_10)) {
//         std::cout << i << '\n';
//     }
// }

}  // namespace iterators
}  // namespace common
}  // namespace gaia
