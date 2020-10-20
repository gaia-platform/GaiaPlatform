/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "server_index_impl.hpp"

#include "hash_index.hpp"
#include "range_index.hpp"
#include "db_server.hpp"
#include "db_shared_data.hpp"

using namespace gaia::common;

namespace gaia
{
namespace db
{
namespace index
{

template <typename T_index>
std::function<std::optional<index_record_t>()> get_record_generator_helper(T_index& idx)
{
    auto iter = idx.begin();
    auto iter_end = idx.end();
    return [iter, iter_end]() mutable -> std::optional<index_record_t> {
        while (iter != iter_end)
        {
            index_record_t record = iter->second;
            ++iter;
            return record;
        }
        // Signal end of iteration.
        return std::nullopt;
    };
}

std::function<std::optional<index_record_t>()> server_index_stream::get_record_generator_for_index(gaia_id_t index_id)
{
    auto it = get_indexes()->find(index_id);

    if (it == get_indexes()->end())
    {
        throw index_not_found(index_id);
    }

    auto index_type = it->second->type();

    switch (index_type)
    {
    case value_index_type_t::range:
    {
        return get_record_generator_helper(static_cast<range_index_t&>(*it->second));
    }
    break;
    case value_index_type_t::hash:
    {
        return get_record_generator_helper(static_cast<hash_index_t&>(*it->second));
    }
    break;
    default:
        throw invalid_index_type();
    }
}

} // namespace index
} // namespace db
} // namespace gaia
