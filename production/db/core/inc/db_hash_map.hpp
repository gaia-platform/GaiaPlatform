/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/exceptions.hpp"

#include "db_internal_types.hpp"
#include "db_shared_data.hpp"

namespace gaia
{
namespace db
{

class db_hash_map
{
public:
    static bool insert(common::gaia_id_t id, gaia_locator_t locator)
    {
        ASSERT_PRECONDITION(id.is_valid(), "Cannot call db_hash_map::insert() with an invalid ID!");
        ASSERT_PRECONDITION(locator.is_valid(), "Cannot call db_hash_map::insert() with an invalid locator!");

        id_index_t* id_index = get_id_index();
        size_t bucket_index = id % c_hash_buckets;

        // Initialize next_index_ptr to point to the head of the linked list of
        // hash nodes for this bucket. (We need a load-seq_cst to ensure
        // linearizability.)
        auto next_index_ptr = &(id_index->list_head_index_for_bucket[bucket_index]);

        while (true)
        {
            // Check if we're at the end of this bucket's linked list. If so,
            // then create a new hash node and try to point the current next
            // pointer to it. If that fails, then find the new tail of this
            // bucket's linked list and point it to the new hash node.
            //
            // The linearization point of an insert is a store-seq_cst to
            // next_index, so we need a load-seq_cst on next_index to ensure
            // linearizability.
            auto next_index = next_index_ptr->load(std::memory_order_seq_cst);
            if (!next_index)
            {
                // We're at the end of this bucket's linked list, so create a
                // new hash node and point the tail of the list to the new node.
                //
                // NB: 0 serves as an "invalid" value for `next_index`, so we
                // can't use the entry `id_index->hash_nodes[0]. This means that
                // `new_hash_node_index` must start at 1.
                auto new_hash_node_index = ++(id_index->last_allocated_hash_node_index);
                ASSERT_INVARIANT(
                    new_hash_node_index < c_max_locators,
                    "Index of new hash node is out of range!");
                auto& new_hash_node = id_index->hash_nodes[new_hash_node_index];

                // These stores can be relaxed, because the new node is
                // inaccessible until the subsequent CAS succeeds, and release
                // semantics ensure that these stores must be visible once the
                // CAS is observed.
                new_hash_node.id.store(id, std::memory_order_relaxed);
                new_hash_node.locator.store(locator, std::memory_order_relaxed);

                // Try to point the current next_index to the new node.
                // NB: A successful CAS requires store-seq_cst to ensure that
                // insert()/find()/remove() are globally linearizable.
                uint32_t expected_index = 0;
                auto desired_index = static_cast<uint32_t>(new_hash_node_index);
                if (next_index_ptr->compare_exchange_strong(expected_index, desired_index))
                {
                    // We pointed the tail node of this bucket's list to the new
                    // node, so we're done.
                    return true;
                }

                // If we didn't succeed in linking the new node to the tail of
                // the list, some other thread must have appended a new node, so
                // continue traversing the list.
                continue;
            }

            // We haven't reached the end of this bucket's linked list yet, so
            // check the ID of the current node and return false if it matches,
            // otherwise continue traversing the list.
            auto& current_node = id_index->hash_nodes[next_index];

            // Because the ID field is always written before a store-seq_cst to
            // next_index, and we've already performed a load-seq_cst on
            // next_index, we can use a relaxed load for the ID.
            common::gaia_id_t current_id = current_node.id.load(std::memory_order_relaxed);

            // We don't assert here because the caller might be performing a blind insert.
            // (Note that we don't allow inserting a different locator under the same ID.)
            if (current_id == id)
            {
                return false;
            }

            next_index_ptr = &current_node.next_index;
        }
    }

    static gaia_locator_t find(common::gaia_id_t id)
    {
        ASSERT_PRECONDITION(id.is_valid(), "Cannot call db_hash_map::find() with an invalid ID!");

        id_index_t* id_index = get_id_index();
        size_t bucket_index = id % c_hash_buckets;

        gaia_locator_t locator;

        // Initialize next_index_ptr to point to the head of the linked list of
        // hash nodes for this bucket. (We need a load-seq_cst to ensure
        // linearizability.)
        auto next_index_ptr = &(id_index->list_head_index_for_bucket[bucket_index]);

        while (true)
        {
            // Check if we're at the end of this bucket's linked list.
            //
            // The linearization point of an insert is a store-seq_cst to
            // next_index, so we need a load-seq_cst on next_index to ensure
            // linearizability.
            auto next_index = next_index_ptr->load(std::memory_order_seq_cst);
            if (!next_index)
            {
                break;
            }

            // We haven't reached the end of this bucket's linked list yet, so
            // check the ID of the current node and return its locator if it
            // matches, otherwise continue traversing the list.
            auto& current_node = id_index->hash_nodes[next_index];

            // Because the ID field is always written before a store-seq_cst to
            // next_index, and we've already performed a load-seq_cst on
            // next_index, we can use a relaxed load for the ID.
            common::gaia_id_t current_id = current_node.id.load(std::memory_order_relaxed);

            if (current_id == id)
            {
                // We need to use a load-seq_cst on the locator field because
                // the linearization point of remove() is a store-seq_cst on the
                // locator field.
                locator = current_node.locator.load(std::memory_order_seq_cst);
                break;
            }

            next_index_ptr = &current_node.next_index;
        }

        return locator;
    }

    static bool remove(common::gaia_id_t id)
    {
        ASSERT_PRECONDITION(id.is_valid(), "Cannot call db_hash_map::remove() with an invalid ID!");

        id_index_t* id_index = get_id_index();
        size_t bucket_index = id % c_hash_buckets;

        // Initialize next_index_ptr to point to the head of the linked list of
        // hash nodes for this bucket. (We need a load-seq_cst to ensure
        // linearizability.)
        auto next_index_ptr = &(id_index->list_head_index_for_bucket[bucket_index]);

        while (true)
        {
            // Check if we're at the end of this bucket's linked list.
            //
            // The linearization point of an insert is a store-seq_cst to
            // next_index, so we need a load-seq_cst on next_index to ensure
            // linearizability.
            auto next_index = next_index_ptr->load(std::memory_order_seq_cst);
            if (!next_index)
            {
                break;
            }

            // We haven't reached the end of this bucket's linked list yet, so
            // check the ID of the current node and return false if it matches,
            // otherwise continue traversing the list.
            auto& current_node = id_index->hash_nodes[next_index];

            // Because the ID field is always written before a store-seq_cst to
            // next_index, and we've already performed a load-seq_cst on
            // next_index, we can use a relaxed load for the ID.
            common::gaia_id_t current_id = current_node.id.load(std::memory_order_relaxed);

            if (current_id == id)
            {
                // We use CAS because the operation should return false if
                // another thread already deleted the locator.
                //
                // A successful CAS should issue a store-seq_cst on the
                // locator field so this operation linearizes with find().
                // An unsuccessful CAS should issue a load-seq_cst on the
                // locator field so the assert is correct.
                auto expected_locator = current_node.locator.load(std::memory_order_relaxed);
                auto desired_locator = c_invalid_gaia_locator;
                if (expected_locator != c_invalid_gaia_locator)
                {
                    if (current_node.locator.compare_exchange_strong(expected_locator, desired_locator))
                    {
                        return true;
                    }

                    // If the CAS failed, it could only be because another thread deleted the locator.
                    ASSERT_INVARIANT(
                        expected_locator == c_invalid_gaia_locator,
                        "Cannot change locator value from a valid value to another valid value!");
                }

                return false;
            }

            next_index_ptr = &current_node.next_index;
        }

        return false;
    }
};

} // namespace db
} // namespace gaia
