/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "retail_assert.hpp"
#include "se_helpers.hpp"
#include "se_shared_data.hpp"
#include "se_types.hpp"

namespace gaia
{
namespace db
{

class se_hash_map
{
public:
    static hash_node* insert(gaia::common::gaia_id_t id)
    {
        locators* locators = gaia::db::get_shared_locators();
        data* data = gaia::db::get_shared_data();
        if (locators == nullptr)
        {
            throw transaction_not_open();
        }

        hash_node* node = data->hash_nodes + (id % c_hash_buckets);
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
                node = data->hash_nodes + node->next_offset;
                continue;
            }

            if (!new_node_idx)
            {
                gaia::common::retail_assert(
                    data->hash_node_count + c_hash_buckets < c_hash_list_elements,
                    "hash_node_count exceeds expected limits!");
                new_node_idx = c_hash_buckets + __sync_fetch_and_add(&data->hash_node_count, 1);
                (data->hash_nodes + new_node_idx)->id = id;
            }

            if (__sync_bool_compare_and_swap(&node->next_offset, 0, new_node_idx))
            {
                return data->hash_nodes + new_node_idx;
            }
        }
    }

    static gaia_locator_t find(gaia::common::gaia_id_t id)
    {
        locators* locators = gaia::db::get_shared_locators();
        data* data = gaia::db::get_shared_data();
        if (locators == nullptr)
        {
            throw transaction_not_open();
        }

        hash_node* node = data->hash_nodes + (id % c_hash_buckets);

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
                ? data->hash_nodes + node->next_offset
                : nullptr;
        }

        return c_invalid_gaia_locator;
    }

    static void remove(gaia::common::gaia_id_t id)
    {
        data* data = gaia::db::get_shared_data();
        hash_node* node = data->hash_nodes + (id % c_hash_buckets);

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
            node = data->hash_nodes + node->next_offset;
        }
    }
};

} // namespace db
} // namespace gaia
