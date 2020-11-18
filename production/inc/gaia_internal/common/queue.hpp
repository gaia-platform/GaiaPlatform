/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <unistd.h>

#include <shared_mutex>

#include "gaia/retail_assert.hpp"

namespace gaia
{
/**
 * \addtogroup Gaia
 * @{
 */
namespace common
{
/**
 * \addtogroup Common
 * @{
 */

template <class T>
struct queue_element_t
{
    T value;

    queue_element_t* next;
    queue_element_t* previous;

    mutable std::shared_mutex lock;

    queue_element_t();
    queue_element_t(T value);
};

template <class T>
class queue_t
{
public:
    queue_t();
    ~queue_t();

    // Insert a value in the queue.
    void enqueue(const T& value);

    // Extract a value from the queue.
    // If the queue is empty, the value will be left unset.
    void dequeue(T& value);

    bool is_empty() const;

protected:
    queue_element_t<T> m_head;
    queue_element_t<T> m_tail;

    // Internal extraction method: returns true if the operation succeeded
    // or false if it was aborted due to another concurrent operation.
    // In the latter case, the caller should retry the operation again.
    bool dequeue_internal(T& value);
};

#include "queue.inc"

/*@}*/
} // namespace common
/*@}*/
} // namespace gaia
