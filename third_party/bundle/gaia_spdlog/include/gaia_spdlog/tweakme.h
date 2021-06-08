// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

///////////////////////////////////////////////////////////////////////////////
//
// Edit this file to squeeze more performance, and to customize supported
// features
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Under Linux, the much faster CLOCK_REALTIME_COARSE clock can be used.
// This clock is less accurate - can be off by dozens of millis - depending on
// the kernel HZ.
// Uncomment to use it instead of the regular clock.
//
// #define GAIA_SPDLOG_CLOCK_COARSE
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Uncomment if thread id logging is not needed (i.e. no %t in the log pattern).
// This will prevent gaia_spdlog from querying the thread id on each log call.
//
// WARNING: If the log pattern contains thread id (i.e, %t) while this flag is
// on, zero will be logged as thread id.
//
// #define GAIA_SPDLOG_NO_THREAD_ID
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Uncomment to prevent gaia_spdlog from using thread local storage.
//
// WARNING: if your program forks, UNCOMMENT this flag to prevent undefined
// thread ids in the children logs.
//
// #define GAIA_SPDLOG_NO_TLS
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Uncomment to avoid gaia_spdlog's usage of atomic log levels
// Use only if your code never modifies a logger's log levels concurrently by
// different threads.
//
// #define GAIA_SPDLOG_NO_ATOMIC_LEVELS
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Uncomment to enable usage of wchar_t for file names on Windows.
//
// #define GAIA_SPDLOG_WCHAR_FILENAMES
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Uncomment to override default eol ("\n" or "\r\n" under Linux/Windows)
//
// #define GAIA_SPDLOG_EOL ";-)\n"
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Uncomment to override default folder separators ("/" or "\\/" under
// Linux/Windows). Each character in the string is treated as a different
// separator.
//
// #define GAIA_SPDLOG_FOLDER_SEPS "\\"
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Uncomment to use your own copy of the gaia_fmt library instead of gaia_spdlog's copy.
// In this case gaia_spdlog will try to include <gaia_fmt/format.h> so set your -I flag
// accordingly.
//
// #define GAIA_SPDLOG_FMT_EXTERNAL
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Uncomment to enable wchar_t support (convert to utf8)
//
// #define GAIA_SPDLOG_WCHAR_TO_UTF8_SUPPORT
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Uncomment to prevent child processes from inheriting log file descriptors
//
// #define GAIA_SPDLOG_PREVENT_CHILD_FD
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Uncomment to customize level names (e.g. "MT TRACE")
//
// #define GAIA_SPDLOG_LEVEL_NAMES { "MY TRACE", "MY DEBUG", "MY INFO", "MY WARNING",
// "MY ERROR", "MY CRITICAL", "OFF" }
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Uncomment to customize short level names (e.g. "MT")
// These can be longer than one character.
//
// #define GAIA_SPDLOG_SHORT_LEVEL_NAMES { "T", "D", "I", "W", "E", "C", "O" }
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Uncomment to disable default logger creation.
// This might save some (very) small initialization time if no default logger is needed.
//
// #define GAIA_SPDLOG_DISABLE_DEFAULT_LOGGER
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Uncomment and set to compile time level with zero cost (default is INFO).
// Macros like GAIA_SPDLOG_DEBUG(..), GAIA_SPDLOG_INFO(..)  will expand to empty statements if not enabled
//
// #define GAIA_SPDLOG_ACTIVE_LEVEL GAIA_SPDLOG_LEVEL_INFO
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Uncomment (and change if desired) macro to use for function names.
// This is compiler dependent.
// __PRETTY_FUNCTION__ might be nicer in clang/gcc, and __FUNCTION__ in msvc.
// Defaults to __FUNCTION__ (should work on all compilers) if not defined.
//
// #define GAIA_SPDLOG_FUNCTION __PRETTY_FUNCTION__
///////////////////////////////////////////////////////////////////////////////
