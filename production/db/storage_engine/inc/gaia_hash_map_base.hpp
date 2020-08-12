/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "storage_engine.hpp"
#include "retail_assert.hpp"

namespace gaia {
namespace db {

using namespace common;

template<class T>
class gaia_hash_map_base {
   public:
    static se_base::hash_node* insert(const gaia_id_t id) {
        if (T::check_no_active_transaction()) {
            throw transaction_not_open();
        }

        se_base::hash_node* node = T::get_hash_node(id % se_base::HASH_BUCKETS);
        if (node->id == 0 && __sync_bool_compare_and_swap(&node->id, 0, id)) {
            return node;
        }

        int64_t new_node_idx = 0;

        for (;;) {
            __sync_synchronize();

            if (node->id == id) {
                if (node->row_id &&
                    T::locator_exists(node->row_id)) {
                    throw duplicate_id(id);
                } else {
                    return node;
                }
            }

            if (node->next) {
                node = T::get_hash_node(node->next);
                continue;
            }

            if (!new_node_idx) {
                retail_assert(*T::get_hash_node_count() + se_base::HASH_BUCKETS < se_base::HASH_LIST_ELEMENTS);
                new_node_idx = se_base::HASH_BUCKETS + __sync_fetch_and_add(T::get_hash_node_count(), 1);
                (T::get_hash_node(new_node_idx))->id = id;
            }

            if (__sync_bool_compare_and_swap(&node->next, 0, new_node_idx)) {
                return T::get_hash_node(new_node_idx);
            }
        }
    }

    static int64_t find(const gaia_id_t id) {
        if (T::check_no_active_transaction()) {
            throw transaction_not_open();
        }

        se_base::hash_node* node = T::get_hash_node(id % se_base::HASH_BUCKETS); 

        while (node) {
            if (node->id == id) {
                if (node->row_id && T::locator_exists(node->row_id)) {
                    return node->row_id;
                } else {
                    return 0;
                }
            }

            node = node->next
                ? T::get_hash_node(node->next)
                : 0;
        }

        return 0;
    }

    static void remove(const gaia_id_t id) {
        se_base::hash_node* node = T::get_hash_node(id % se_base::HASH_BUCKETS); 

        while (node->id) {
            if (node->id == id) {
                if (node->row_id) {
                    node->row_id = 0;
                }
                return;
            }
            if (!node->next) {
                return;
            }
            node = T::get_hash_node(node->next);
        }
    }
};

}  // namespace db
}  // namespace gaia
