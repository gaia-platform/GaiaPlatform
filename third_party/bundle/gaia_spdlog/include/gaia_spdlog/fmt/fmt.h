//
// Copyright(c) 2016-2018 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#pragma once

//
// Include a bundled header-only copy of gaia_fmtlib or an external one.
// By default gaia_spdlog include its own copy.
//

#if !defined(GAIA_SPDLOG_FMT_EXTERNAL)
#if !defined(GAIA_SPDLOG_COMPILED_LIB) && !defined(GAIA_FMT_HEADER_ONLY)
#define GAIA_FMT_HEADER_ONLY
#endif
#ifndef GAIA_FMT_USE_WINDOWS_H
#define GAIA_FMT_USE_WINDOWS_H 0
#endif
// enable the 'n' flag in for backward compatibility with gaia_fmt 6.x
#define GAIA_FMT_DEPRECATED_N_SPECIFIER
#include <gaia_spdlog/fmt/bundled/core.h>
#include <gaia_spdlog/fmt/bundled/format.h>
#else // GAIA_SPDLOG_FMT_EXTERNAL is defined - use external gaia_fmtlib
#include <gaia_fmt/core.h>
#include <gaia_fmt/format.h>
#endif
