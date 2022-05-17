/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "client_contexts.hpp"

#include "index.hpp"

namespace gaia
{
namespace db
{

void client_transaction_context_t::clear()
{
    // Clear the local indexes.
    for (const auto& index : local_indexes)
    {
        index.second->clear();
    }
}

void client_session_context_t::clear()
{
}

} // namespace db
} // namespace gaia
