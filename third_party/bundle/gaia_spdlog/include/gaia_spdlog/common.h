// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <gaia_spdlog/tweakme.h>
#include <gaia_spdlog/details/null_mutex.h>

#include <atomic>
#include <chrono>
#include <initializer_list>
#include <memory>
#include <exception>
#include <string>
#include <type_traits>
#include <functional>

#ifdef GAIA_SPDLOG_COMPILED_LIB
#undef GAIA_SPDLOG_HEADER_ONLY
#if defined(_WIN32) && defined(GAIA_SPDLOG_SHARED_LIB)
#ifdef spdlog_EXPORTS
#define GAIA_SPDLOG_API __declspec(dllexport)
#else
#define GAIA_SPDLOG_API __declspec(dllimport)
#endif
#else // !defined(_WIN32) || !defined(GAIA_SPDLOG_SHARED_LIB)
#define GAIA_SPDLOG_API
#endif
#define GAIA_SPDLOG_INLINE
#else // !defined(GAIA_SPDLOG_COMPILED_LIB)
#define GAIA_SPDLOG_API
#define GAIA_SPDLOG_HEADER_ONLY
#define GAIA_SPDLOG_INLINE inline
#endif // #ifdef GAIA_SPDLOG_COMPILED_LIB

#include <gaia_spdlog/fmt/fmt.h>

// visual studio upto 2013 does not support noexcept nor constexpr
#if defined(_MSC_VER) && (_MSC_VER < 1900)
#define GAIA_SPDLOG_NOEXCEPT _NOEXCEPT
#define GAIA_SPDLOG_CONSTEXPR
#else
#define GAIA_SPDLOG_NOEXCEPT noexcept
#define GAIA_SPDLOG_CONSTEXPR constexpr
#endif

#if defined(__GNUC__) || defined(__clang__)
#define GAIA_SPDLOG_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define GAIA_SPDLOG_DEPRECATED __declspec(deprecated)
#else
#define GAIA_SPDLOG_DEPRECATED
#endif

// disable thread local on msvc 2013
#ifndef GAIA_SPDLOG_NO_TLS
#if (defined(_MSC_VER) && (_MSC_VER < 1900)) || defined(__cplusplus_winrt)
#define GAIA_SPDLOG_NO_TLS 1
#endif
#endif

#ifndef GAIA_SPDLOG_FUNCTION
#define GAIA_SPDLOG_FUNCTION static_cast<const char *>(__FUNCTION__)
#endif

#ifdef GAIA_SPDLOG_NO_EXCEPTIONS
#define GAIA_SPDLOG_TRY
#define GAIA_SPDLOG_THROW(ex)                                                                                                                   \
    do                                                                                                                                     \
    {                                                                                                                                      \
        printf("gaia_spdlog fatal error: %s\n", ex.what());                                                                                     \
        std::abort();                                                                                                                      \
    } while (0)
#define GAIA_SPDLOG_CATCH_ALL()
#else
#define GAIA_SPDLOG_TRY try
#define GAIA_SPDLOG_THROW(ex) throw(ex)
#define GAIA_SPDLOG_CATCH_ALL() catch (...)
#endif

namespace gaia_spdlog {

class formatter;

namespace sinks {
class sink;
}

#if defined(_WIN32) && defined(GAIA_SPDLOG_WCHAR_FILENAMES)
using filename_t = std::wstring;
// allow macro expansion to occur in GAIA_SPDLOG_FILENAME_T
#define GAIA_SPDLOG_FILENAME_T_INNER(s) L##s
#define GAIA_SPDLOG_FILENAME_T(s) GAIA_SPDLOG_FILENAME_T_INNER(s)
#else
using filename_t = std::string;
#define GAIA_SPDLOG_FILENAME_T(s) s
#endif

using log_clock = std::chrono::system_clock;
using sink_ptr = std::shared_ptr<sinks::sink>;
using sinks_init_list = std::initializer_list<sink_ptr>;
using err_handler = std::function<void(const std::string &err_msg)>;
using string_view_t = gaia_fmt::basic_string_view<char>;
using wstring_view_t = gaia_fmt::basic_string_view<wchar_t>;
using memory_buf_t = gaia_fmt::basic_memory_buffer<char, 250>;
using wmemory_buf_t = gaia_fmt::basic_memory_buffer<wchar_t, 250>;

#ifdef GAIA_SPDLOG_WCHAR_TO_UTF8_SUPPORT
#ifndef _WIN32
#error GAIA_SPDLOG_WCHAR_TO_UTF8_SUPPORT only supported on windows
#else
template<typename T>
struct is_convertible_to_wstring_view : std::is_convertible<T, wstring_view_t>
{};
#endif // _WIN32
#else
template<typename>
struct is_convertible_to_wstring_view : std::false_type
{};
#endif // GAIA_SPDLOG_WCHAR_TO_UTF8_SUPPORT

#if defined(GAIA_SPDLOG_NO_ATOMIC_LEVELS)
using level_t = details::null_atomic_int;
#else
using level_t = std::atomic<int>;
#endif

#define GAIA_SPDLOG_LEVEL_TRACE 0
#define GAIA_SPDLOG_LEVEL_DEBUG 1
#define GAIA_SPDLOG_LEVEL_INFO 2
#define GAIA_SPDLOG_LEVEL_WARN 3
#define GAIA_SPDLOG_LEVEL_ERROR 4
#define GAIA_SPDLOG_LEVEL_CRITICAL 5
#define GAIA_SPDLOG_LEVEL_OFF 6

#if !defined(GAIA_SPDLOG_ACTIVE_LEVEL)
#define GAIA_SPDLOG_ACTIVE_LEVEL GAIA_SPDLOG_LEVEL_INFO
#endif

// Log level enum
namespace level {
enum level_enum
{
    trace = GAIA_SPDLOG_LEVEL_TRACE,
    debug = GAIA_SPDLOG_LEVEL_DEBUG,
    info = GAIA_SPDLOG_LEVEL_INFO,
    warn = GAIA_SPDLOG_LEVEL_WARN,
    err = GAIA_SPDLOG_LEVEL_ERROR,
    critical = GAIA_SPDLOG_LEVEL_CRITICAL,
    off = GAIA_SPDLOG_LEVEL_OFF,
    n_levels
};

#if !defined(GAIA_SPDLOG_LEVEL_NAMES)
#define GAIA_SPDLOG_LEVEL_NAMES                                                                                                                 \
    {                                                                                                                                      \
        "trace", "debug", "info", "warning", "error", "critical", "off"                                                                    \
    }
#endif

#if !defined(GAIA_SPDLOG_SHORT_LEVEL_NAMES)

#define GAIA_SPDLOG_SHORT_LEVEL_NAMES                                                                                                           \
    {                                                                                                                                      \
        "T", "D", "I", "W", "E", "C", "O"                                                                                                  \
    }
#endif

GAIA_SPDLOG_API const string_view_t &to_string_view(gaia_spdlog::level::level_enum l) GAIA_SPDLOG_NOEXCEPT;
GAIA_SPDLOG_API void set_string_view(gaia_spdlog::level::level_enum l, const string_view_t &s) GAIA_SPDLOG_NOEXCEPT;
GAIA_SPDLOG_API const char *to_short_c_str(gaia_spdlog::level::level_enum l) GAIA_SPDLOG_NOEXCEPT;
GAIA_SPDLOG_API gaia_spdlog::level::level_enum from_str(const std::string &name) GAIA_SPDLOG_NOEXCEPT;

} // namespace level

//
// Color mode used by sinks with color support.
//
enum class color_mode
{
    always,
    automatic,
    never
};

//
// Pattern time - specific time getting to use for pattern_formatter.
// local time by default
//
enum class pattern_time_type
{
    local, // log localtime
    utc    // log utc
};

//
// Log exception
//
class GAIA_SPDLOG_API gaia_spdlog_ex : public std::exception
{
public:
    explicit gaia_spdlog_ex(std::string msg);
    gaia_spdlog_ex(const std::string &msg, int last_errno);
    const char *what() const GAIA_SPDLOG_NOEXCEPT override;

private:
    std::string msg_;
};

[[noreturn]] GAIA_SPDLOG_API void throw_spdlog_ex(const std::string &msg, int last_errno);
[[noreturn]] GAIA_SPDLOG_API void throw_spdlog_ex(std::string msg);

struct source_loc
{
    GAIA_SPDLOG_CONSTEXPR source_loc() = default;
    GAIA_SPDLOG_CONSTEXPR source_loc(const char *filename_in, int line_in, const char *funcname_in)
        : filename{filename_in}
        , line{line_in}
        , funcname{funcname_in}
    {}

    GAIA_SPDLOG_CONSTEXPR bool empty() const GAIA_SPDLOG_NOEXCEPT
    {
        return line == 0;
    }
    const char *filename{nullptr};
    int line{0};
    const char *funcname{nullptr};
};

namespace details {
// make_unique support for pre c++14

#if __cplusplus >= 201402L // C++14 and beyond
using std::make_unique;
#else
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&...args)
{
    static_assert(!std::is_array<T>::value, "arrays not supported");
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#endif
} // namespace details
} // namespace gaia_spdlog

#ifdef GAIA_SPDLOG_HEADER_ONLY
#include "common-inl.h"
#endif
