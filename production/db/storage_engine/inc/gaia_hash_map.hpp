/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "storage_engine.hpp"

namespace gaia {
namespace db {

using namespace common;

class gaia_hash_map {
public:
    static hash_node* insert(se_base::data* s_data, se_base::locators* locators, const gaia_id_t id) {
        if (locators == nullptr) {
            throw transaction_not_open();
        }

        hash_node* node = s_data->hash_nodes + (id % se_base::HASH_BUCKETS);
        if (node->id == 0 && __sync_bool_compare_and_swap(&node->id, 0, id)) {
            return node;
        }

        size_t new_node_idx = 0;

        for (;;) {
            __sync_synchronize();

            if (node->id == id) {
                if (node->locator && se_base::locator_exists(locators, node->locator)) {
                    throw duplicate_id(id);
                } else {
                    return node;
                }
            }

            if (node->next_offset) {
                node = s_data->hash_nodes + node->next_offset;
                continue;
            }

            if (!new_node_idx) {
                retail_assert(s_data->hash_node_count + se_base::HASH_BUCKETS < se_base::HASH_LIST_ELEMENTS);
                new_node_idx = se_base::HASH_BUCKETS + __sync_fetch_and_add(&s_data->hash_node_count, 1);
                (s_data->hash_nodes + new_node_idx)->id = id;
            }

            if (__sync_bool_compare_and_swap(&node->next_offset, 0, new_node_idx)) {
                return s_data->hash_nodes + new_node_idx;
            }
        }
    }

    static int64_t find(se_base::data* s_data, se_base::locators* locators, const gaia_id_t id) {
        if (locators == nullptr) {
            throw transaction_not_open();
        }

        hash_node* node = s_data->hash_nodes + (id % se_base::HASH_BUCKETS);

        while (node) {
            if (node->id == id) {
                if (node->locator && se_base::locator_exists(locators, node->locator)) {
                    return node->locator;
                } else {
                    return 0;
                }
            }

            node = node->next_offset
                ? s_data->hash_nodes + node->next_offset
                : 0;
        }

        return 0;
    }

    static void remove(se_base::data* s_data, const gaia_id_t id) {
        hash_node* node = s_data->hash_nodes + (id % se_base::HASH_BUCKETS);

        while (node->id) {
            if (node->id == id) {
                if (node->locator) {
                    node->locator = 0;
                }
                return;
            }
            if (!node->next_offset) {
                return;
            }
            node = s_data->hash_nodes + node->next_offset;
        }
    }
};

} // namespace db
} // namespace gaia
