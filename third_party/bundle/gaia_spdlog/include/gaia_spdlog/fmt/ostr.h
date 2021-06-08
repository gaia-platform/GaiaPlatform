//
// Copyright(c) 2016 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#pragma once
//
// include bundled or external copy of gaia_fmtlib's ostream support
//

#if !defined(GAIA_SPDLOG_FMT_EXTERNAL)
#ifdef GAIA_SPDLOG_HEADER_ONLY
#ifndef GAIA_FMT_HEADER_ONLY
#define GAIA_FMT_HEADER_ONLY
#endif
#endif
#include <gaia_spdlog/fmt/bundled/ostream.h>
#else
#include <gaia_fmt/ostream.h>
#endif
