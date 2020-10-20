/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Temporary structures/definitions for implementation of server-side indexes.
// This file should be removed when shared-memory friendly indexes are implemented.

#pragma once

#include <functional>
#include <unordered_map>
#include <utility>

#include "gaia/common.hpp"
#include "index_builder.hpp"

namespace gaia
{
namespace db
{
namespace index
{

class server_index_stream
{
public:
    static std::function<std::optional<index_record_t>()> get_record_generator_for_index(common::gaia_id_t index_id);
};

} // namespace index
} // namespace db
} // namespace gaia
