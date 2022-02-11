////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <cstdint>

namespace gaia::clearpath::vertex_type
{

static constexpr int64_t c_lidar_scan = 0;
static constexpr int64_t c_submap = 1;
static constexpr int64_t c_image_keyframe = 2;
static constexpr int64_t c_visual_map_point = 3;
static constexpr int64_t c_visual_landmark = 4;
static constexpr int64_t c_magnetic_line = 5;

} // namespace gaia::clearpath::vertex_type
