/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/exceptions.hpp"

#include "db_helpers.hpp"
#include "db_internal_types.hpp"
#include "db_shared_data.hpp"

namespace gaia
{
namespace db
{

class db_hash_map
{
public:
    static hash_node_t* insert(common::gaia_id_t id)
    {
        locators_t* locators = get_locators();
        id_index_t* id_index = get_id_index();
        if (locators == nullptr)
        {
            throw no_open_transaction_internal();
        }

        hash_node_t* node = id_index->hash_nodes + (id % c_hash_buckets);
        common::gaia_id_t::value_type expected_id = common::c_invalid_gaia_id;
        if (node->id.compare_exchange_strong(expected_id, id))
        {
            return node;
        }

        size_t new_node_idx = 0;

        for (;;)
        {
            if (node->id == id)
            {
                if (locator_exists(node->locator.load()))
                {
                    throw duplicate_object_id_internal(id);
                }
                else
                {
                    return node;
                }
            }

            if (node->next_offset)
            {
                node = id_index->hash_nodes + node->next_offset;
                continue;
            }

            if (!new_node_idx)
            {
                ASSERT_INVARIANT(
                    id_index->hash_node_count + c_hash_buckets < c_max_locators,
                    "hash_node_count exceeds expected limits!");
                new_node_idx = c_hash_buckets + id_index->hash_node_count++;
                (id_index->hash_nodes + new_node_idx)->id = id;
            }

            size_t expected_offset = 0;
            if (node->next_offset.compare_exchange_strong(expected_offset, new_node_idx))
            {
                return id_index->hash_nodes + new_node_idx;
            }
        }
    }

    static gaia_locator_t find(common::gaia_id_t id)
    {
        locators_t* locators = get_locators();
        id_index_t* id_index = get_id_index();
        if (locators == nullptr)
        {
            throw no_open_transaction_internal();
        }

        hash_node_t* node = id_index->hash_nodes + (id % c_hash_buckets);

        while (node)
        {
            if (node->id == id)
            {
                auto locator = node->locator.load();
                if (locator_exists(locator))
                {
                    return locator;
                }
                else
                {
                    return c_invalid_gaia_locator;
                }
            }

            node = node->next_offset
                ? id_index->hash_nodes + node->next_offset
                : nullptr;
        }

        return c_invalid_gaia_locator;
    }

    static void remove(common::gaia_id_t id)
    {
        id_index_t* id_index = get_id_index();
        hash_node_t* node = id_index->hash_nodes + (id % c_hash_buckets);

        while (node->id != common::c_invalid_gaia_id)
        {
            if (node->id == id)
            {
                node->locator = c_invalid_gaia_locator;
                return;
            }
            if (!node->next_offset)
            {
                return;
            }
            node = id_index->hash_nodes + node->next_offset;
        }
    }
};

} // namespace db
} // namespace gaia
