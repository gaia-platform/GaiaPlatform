//
// Copyright(c) 2016 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#pragma once
//
// include bundled or external copy of fmtlib's chrono support
//

#if !defined(GAIA_SPDLOG_FMT_EXTERNAL)
#ifdef GAIA_SPDLOG_HEADER_ONLY
#ifndef FMT_HEADER_ONLY
#define FMT_HEADER_ONLY
#endif
#endif
#include <gaia_spdlog/fmt/bundled/chrono.h>
#else
#include <fmt/chrono.h>
#endif
