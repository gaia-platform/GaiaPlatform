// Slightly modified version of fmt lib's format.cc source file.
// Copyright (c) 2012 - 2016, Victor Zverovich
// All rights reserved.

#ifndef GAIA_SPDLOG_COMPILED_LIB
#error Please define GAIA_SPDLOG_COMPILED_LIB to compile this file.
#endif

#if !defined(GAIA_SPDLOG_FMT_EXTERNAL)
#include <gaia_spdlog/fmt/bundled/format-inl.h>


GAIA_FMT_BEGIN_NAMESPACE
namespace detail {

template <typename T>
int format_float(char* buf, std::size_t size, const char* format, int precision,
                 T value) {
#ifdef GAIA_FMT_FUZZ
  if (precision > 100000)
    throw std::runtime_error(
        "fuzz mode - avoid large allocation inside snprintf");
#endif
  // Suppress the warning about nonliteral format string.
  int (*snprintf_ptr)(char*, size_t, const char*, ...) = GAIA_FMT_SNPRINTF;
  return precision < 0 ? snprintf_ptr(buf, size, format, value)
                       : snprintf_ptr(buf, size, format, precision, value);
}

template GAIA_FMT_API dragonbox::decimal_fp<float> dragonbox::to_decimal(float x)
    GAIA_FMT_NOEXCEPT;
template GAIA_FMT_API dragonbox::decimal_fp<double> dragonbox::to_decimal(double x)
    GAIA_FMT_NOEXCEPT;

// DEPRECATED! This function exists for ABI compatibility.
template <typename Char>
typename basic_format_context<std::back_insert_iterator<buffer<Char>>,
                              Char>::iterator
vformat_to(buffer<Char>& buf, basic_string_view<Char> format_str,
           basic_format_args<basic_format_context<
               std::back_insert_iterator<buffer<type_identity_t<Char>>>,
               type_identity_t<Char>>>
               args) {
  using iterator = std::back_insert_iterator<buffer<char>>;
  using context = basic_format_context<
      std::back_insert_iterator<buffer<type_identity_t<Char>>>,
      type_identity_t<Char>>;
  auto out = iterator(buf);
  format_handler<iterator, Char, context> h(out, format_str, args, {});
  parse_format_string<false>(format_str, h);
  return out;
}
template basic_format_context<std::back_insert_iterator<buffer<char>>,
                              char>::iterator
vformat_to(buffer<char>&, string_view,
           basic_format_args<basic_format_context<
               std::back_insert_iterator<buffer<type_identity_t<char>>>,
               type_identity_t<char>>>);
}  // namespace detail

template struct GAIA_FMT_INSTANTIATION_DEF_API detail::basic_data<void>;

// Workaround a bug in MSVC2013 that prevents instantiation of format_float.
int (*instantiate_format_float)(double, int, detail::float_specs,
                                detail::buffer<char>&) = detail::format_float;

#ifndef GAIA_FMT_STATIC_THOUSANDS_SEPARATOR
template GAIA_FMT_API detail::locale_ref::locale_ref(const std::locale& loc);
template GAIA_FMT_API std::locale detail::locale_ref::get<std::locale>() const;
#endif

// Explicit instantiations for char.

template GAIA_FMT_API std::string detail::grouping_impl<char>(locale_ref);
template GAIA_FMT_API char detail::thousands_sep_impl(locale_ref);
template GAIA_FMT_API char detail::decimal_point_impl(locale_ref);

template GAIA_FMT_API void detail::buffer<char>::append(const char*, const char*);

template GAIA_FMT_API void detail::vformat_to(
    detail::buffer<char>&, string_view,
    basic_format_args<GAIA_FMT_BUFFER_CONTEXT(char)>, detail::locale_ref);

template GAIA_FMT_API int detail::snprintf_float(double, int, detail::float_specs,
                                            detail::buffer<char>&);
template GAIA_FMT_API int detail::snprintf_float(long double, int,
                                            detail::float_specs,
                                            detail::buffer<char>&);
template GAIA_FMT_API int detail::format_float(double, int, detail::float_specs,
                                          detail::buffer<char>&);
template GAIA_FMT_API int detail::format_float(long double, int, detail::float_specs,
                                          detail::buffer<char>&);

// Explicit instantiations for wchar_t.

template GAIA_FMT_API std::string detail::grouping_impl<wchar_t>(locale_ref);
template GAIA_FMT_API wchar_t detail::thousands_sep_impl(locale_ref);
template GAIA_FMT_API wchar_t detail::decimal_point_impl(locale_ref);

template GAIA_FMT_API void detail::buffer<wchar_t>::append(const wchar_t*,
                                                      const wchar_t*);
GAIA_FMT_END_NAMESPACE
#endif // !GAIA_SPDLOG_GAIA_FMT_EXTERNAL
