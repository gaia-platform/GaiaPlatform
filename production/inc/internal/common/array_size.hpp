/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

namespace gaia {
namespace common {

template <typename T, std::size_t N>
constexpr std::size_t array_size(T const (&)[N]) noexcept {
    return N;
}

}  // namespace common
}  // namespace gaia
