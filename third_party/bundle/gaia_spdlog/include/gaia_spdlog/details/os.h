// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <gaia_spdlog/common.h>
#include <ctime> // std::time_t

namespace gaia_spdlog {
namespace details {
namespace os {

GAIA_SPDLOG_API gaia_spdlog::log_clock::time_point now() GAIA_SPDLOG_NOEXCEPT;

GAIA_SPDLOG_API std::tm localtime(const std::time_t &time_tt) GAIA_SPDLOG_NOEXCEPT;

GAIA_SPDLOG_API std::tm localtime() GAIA_SPDLOG_NOEXCEPT;

GAIA_SPDLOG_API std::tm gmtime(const std::time_t &time_tt) GAIA_SPDLOG_NOEXCEPT;

GAIA_SPDLOG_API std::tm gmtime() GAIA_SPDLOG_NOEXCEPT;

// eol definition
#if !defined(GAIA_SPDLOG_EOL)
#ifdef _WIN32
#define GAIA_SPDLOG_EOL "\r\n"
#else
#define GAIA_SPDLOG_EOL "\n"
#endif
#endif

GAIA_SPDLOG_CONSTEXPR static const char *default_eol = GAIA_SPDLOG_EOL;

// folder separator
#if !defined(GAIA_SPDLOG_FOLDER_SEPS)
#ifdef _WIN32
#define GAIA_SPDLOG_FOLDER_SEPS "\\/"
#else
#define GAIA_SPDLOG_FOLDER_SEPS "/"
#endif
#endif

GAIA_SPDLOG_CONSTEXPR static const char folder_seps[] = GAIA_SPDLOG_FOLDER_SEPS;
GAIA_SPDLOG_CONSTEXPR static const filename_t::value_type folder_seps_filename[] = GAIA_SPDLOG_FILENAME_T(GAIA_SPDLOG_FOLDER_SEPS);

// fopen_s on non windows for writing
GAIA_SPDLOG_API bool fopen_s(FILE **fp, const filename_t &filename, const filename_t &mode);

// Remove filename. return 0 on success
GAIA_SPDLOG_API int remove(const filename_t &filename) GAIA_SPDLOG_NOEXCEPT;

// Remove file if exists. return 0 on success
// Note: Non atomic (might return failure to delete if concurrently deleted by other process/thread)
GAIA_SPDLOG_API int remove_if_exists(const filename_t &filename) GAIA_SPDLOG_NOEXCEPT;

GAIA_SPDLOG_API int rename(const filename_t &filename1, const filename_t &filename2) GAIA_SPDLOG_NOEXCEPT;

// Return if file exists.
GAIA_SPDLOG_API bool path_exists(const filename_t &filename) GAIA_SPDLOG_NOEXCEPT;

// Return file size according to open FILE* object
GAIA_SPDLOG_API size_t filesize(FILE *f);

// Return utc offset in minutes or throw gaia_spdlog_ex on failure
GAIA_SPDLOG_API int utc_minutes_offset(const std::tm &tm = details::os::localtime());

// Return current thread id as size_t
// It exists because the std::this_thread::get_id() is much slower(especially
// under VS 2013)
GAIA_SPDLOG_API size_t _thread_id() GAIA_SPDLOG_NOEXCEPT;

// Return current thread id as size_t (from thread local storage)
GAIA_SPDLOG_API size_t thread_id() GAIA_SPDLOG_NOEXCEPT;

// This is avoid msvc issue in sleep_for that happens if the clock changes.
// See https://github.com/gabime/gaia_spdlog/issues/609
GAIA_SPDLOG_API void sleep_for_millis(int milliseconds) GAIA_SPDLOG_NOEXCEPT;

GAIA_SPDLOG_API std::string filename_to_str(const filename_t &filename);

GAIA_SPDLOG_API int pid() GAIA_SPDLOG_NOEXCEPT;

// Determine if the terminal supports colors
// Source: https://github.com/agauniyal/rang/
GAIA_SPDLOG_API bool is_color_terminal() GAIA_SPDLOG_NOEXCEPT;

// Determine if the terminal attached
// Source: https://github.com/agauniyal/rang/
GAIA_SPDLOG_API bool in_terminal(FILE *file) GAIA_SPDLOG_NOEXCEPT;

#if (defined(GAIA_SPDLOG_WCHAR_TO_UTF8_SUPPORT) || defined(GAIA_SPDLOG_WCHAR_FILENAMES)) && defined(_WIN32)
GAIA_SPDLOG_API void wstr_to_utf8buf(wstring_view_t wstr, memory_buf_t &target);

GAIA_SPDLOG_API void utf8_to_wstrbuf(string_view_t str, wmemory_buf_t &target);
#endif

// Return directory name from given path or empty string
// "abc/file" => "abc"
// "abc/" => "abc"
// "abc" => ""
// "abc///" => "abc//"
GAIA_SPDLOG_API filename_t dir_name(filename_t path);

// Create a dir from the given path.
// Return true if succeeded or if this dir already exists.
GAIA_SPDLOG_API bool create_dir(filename_t path);

// non thread safe, cross platform getenv/getenv_s
// return empty string if field not found
GAIA_SPDLOG_API std::string getenv(const char *field);

} // namespace os
} // namespace details
} // namespace gaia_spdlog

#ifdef GAIA_SPDLOG_HEADER_ONLY
#include "os-inl.h"
#endif
