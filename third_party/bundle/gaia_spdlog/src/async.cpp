// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef GAIA_SPDLOG_COMPILED_LIB
#error Please define GAIA_SPDLOG_COMPILED_LIB to compile this file.
#endif

#include <gaia_spdlog/async.h>
#include <gaia_spdlog/async_logger-inl.h>
#include <gaia_spdlog/details/periodic_worker-inl.h>
#include <gaia_spdlog/details/thread_pool-inl.h>

template class GAIA_SPDLOG_API gaia_spdlog::details::mpmc_blocking_queue<gaia_spdlog::details::async_msg>;
