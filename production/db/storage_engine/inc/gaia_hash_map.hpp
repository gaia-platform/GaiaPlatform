/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "storage_engine.hpp"

namespace gaia
{
namespace db
{

using namespace common;

class gaia_hash_map
{
    using hash_node = se_base::hash_node;

public:
    static hash_node* insert(se_base::data* data, se_base::locators* locators, gaia_id_t id)
    {
        if (locators == nullptr)
        {
            throw transaction_not_open();
        }

        hash_node* node = data->hash_nodes + (id % se_base::c_hash_buckets);
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
                if (node->locator && se_base::locator_exists(locators, node->locator))
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
                retail_assert(
                    data->hash_node_count + se_base::c_hash_buckets < se_base::c_hash_list_elements,
                    "hash_node_count exceeds expected limits!");
                new_node_idx = se_base::c_hash_buckets + __sync_fetch_and_add(&data->hash_node_count, 1);
                (data->hash_nodes + new_node_idx)->id = id;
            }

            if (__sync_bool_compare_and_swap(&node->next_offset, 0, new_node_idx))
            {
                return data->hash_nodes + new_node_idx;
            }
        }
    }

    static gaia_locator_t find(se_base::data* data, se_base::locators* locators, gaia_id_t id)
    {
        if (locators == nullptr)
        {
            throw transaction_not_open();
        }

        hash_node* node = data->hash_nodes + (id % se_base::c_hash_buckets);

        while (node)
        {
            if (node->id == id)
            {
                if (node->locator && se_base::locator_exists(locators, node->locator))
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

    static void remove(se_base::data* data, gaia_id_t id)
    {
        hash_node* node = data->hash_nodes + (id % se_base::c_hash_buckets);

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
