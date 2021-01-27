/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "db_helpers.hpp"
#include "db_internal_types.hpp"
#include "db_shared_data.hpp"

#include "gaia_internal/common/retail_assert.hpp"

namespace gaia
{
namespace db
{

class db_hash_map
{
public:
    static hash_node_t* insert(gaia::common::gaia_id_t id)
    {
        locators_t* locators = gaia::db::get_shared_locators();
        shared_id_index_t* id_index = gaia::db::get_shared_id_index();
        if (locators == nullptr)
        {
            throw no_open_transaction();
        }

        hash_node_t* node = id_index->hash_nodes + (id % c_hash_buckets);
        if (node->id == 0 && __sync_bool_compare_and_swap(&node->id, 0, id))
        {
            return node;
        }

        size_t new_node_idx = 0;

        for (;;)
        {
            __sync_synchronize();

            if (node->id == id)
            {
                if (locator_exists(node->locator))
                {
                    throw duplicate_id(id);
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
                gaia::common::retail_assert(
                    id_index->hash_node_count + c_hash_buckets < c_max_locators,
                    "hash_node_count exceeds expected limits!");
                new_node_idx = c_hash_buckets + __sync_fetch_and_add(&id_index->hash_node_count, 1);
                (id_index->hash_nodes + new_node_idx)->id = id;
            }

            if (__sync_bool_compare_and_swap(&node->next_offset, 0, new_node_idx))
            {
                return id_index->hash_nodes + new_node_idx;
            }
        }
    }

    static gaia_locator_t find(gaia::common::gaia_id_t id)
    {
        locators_t* locators = gaia::db::get_shared_locators();
        shared_id_index_t* id_index = gaia::db::get_shared_id_index();
        if (locators == nullptr)
        {
            throw no_open_transaction();
        }

        hash_node_t* node = id_index->hash_nodes + (id % c_hash_buckets);

        while (node)
        {
            if (node->id == id)
            {
                if (locator_exists(node->locator))
                {
                    return node->locator;
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

    static void remove(gaia::common::gaia_id_t id)
    {
        shared_id_index_t* id_index = gaia::db::get_shared_id_index();
        hash_node_t* node = id_index->hash_nodes + (id % c_hash_buckets);

        while (node->id)
        {
            if (node->id == id)
            {
                if (node->locator)
                {
                    node->locator = 0;
                }
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
