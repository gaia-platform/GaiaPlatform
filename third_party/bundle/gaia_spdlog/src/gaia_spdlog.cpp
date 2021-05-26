// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef GAIA_SPDLOG_COMPILED_LIB
#error Please define GAIA_SPDLOG_COMPILED_LIB to compile this file.
#endif

#include <gaia_spdlog/spdlog-inl.h>
#include <gaia_spdlog/common-inl.h>
#include <gaia_spdlog/details/backtracer-inl.h>
#include <gaia_spdlog/details/registry-inl.h>
#include <gaia_spdlog/details/os-inl.h>
#include <gaia_spdlog/pattern_formatter-inl.h>
#include <gaia_spdlog/details/log_msg-inl.h>
#include <gaia_spdlog/details/log_msg_buffer-inl.h>
#include <gaia_spdlog/logger-inl.h>
#include <gaia_spdlog/sinks/sink-inl.h>
#include <gaia_spdlog/sinks/base_sink-inl.h>
#include <gaia_spdlog/details/null_mutex.h>

#include <mutex>

// template instantiate logger constructor with sinks init list
template GAIA_SPDLOG_API gaia_spdlog::logger::logger(std::string name, sinks_init_list::iterator begin, sinks_init_list::iterator end);
template class GAIA_SPDLOG_API gaia_spdlog::sinks::base_sink<std::mutex>;
template class GAIA_SPDLOG_API gaia_spdlog::sinks::base_sink<gaia_spdlog::details::null_mutex>;
