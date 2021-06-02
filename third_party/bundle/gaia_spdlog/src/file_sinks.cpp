// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef GAIA_SPDLOG_COMPILED_LIB
#error Please define GAIA_SPDLOG_COMPILED_LIB to compile this file.
#endif

#include <gaia_spdlog/details/null_mutex.h>
#include <gaia_spdlog/details/file_helper-inl.h>
#include <gaia_spdlog/sinks/basic_file_sink-inl.h>
#include <gaia_spdlog/sinks/base_sink-inl.h>

#include <mutex>

template class GAIA_SPDLOG_API gaia_spdlog::sinks::basic_file_sink<std::mutex>;
template class GAIA_SPDLOG_API gaia_spdlog::sinks::basic_file_sink<gaia_spdlog::details::null_mutex>;

#include <gaia_spdlog/sinks/rotating_file_sink-inl.h>
template class GAIA_SPDLOG_API gaia_spdlog::sinks::rotating_file_sink<std::mutex>;
template class GAIA_SPDLOG_API gaia_spdlog::sinks::rotating_file_sink<gaia_spdlog::details::null_mutex>;
