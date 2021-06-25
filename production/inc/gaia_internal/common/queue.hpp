/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <unistd.h>

#include <atomic>
#include <mutex>
#include <shared_mutex>

#include "gaia_internal/common/retail_assert.hpp"

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

// This is a general-purpose queue implementation
// with synchronization performed using mutexes at node level.
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
    // The caller should properly initialize the value before this call,
    // to be able to determine if it was actually set.
    void dequeue(T& value);

    inline bool is_empty() const;

    inline size_t size() const;

protected:
    queue_element_t<T> m_head;
    queue_element_t<T> m_tail;

    std::atomic<size_t> m_size;

    // Internal extraction method: returns true if the operation succeeded
    // or false if it was aborted due to another concurrent operation.
    // In the latter case, the caller should retry the operation again.
    bool dequeue_internal(T& value);
};

template <class T>
struct mpsc_queue_node_t
{
    mpsc_queue_node_t* volatile next;
    T value;

    mpsc_queue_node_t();
    mpsc_queue_node_t(T value);
};

// This implementation of a multiple-producer single-consumer (mpsc) queue
// is based on the implementation described at:
// https://www.1024cores.net/home/lock-free-algorithms/queues/non-intrusive-mpsc-node-based-queue
template <class T>
class mpsc_queue_t
{
public:
    mpsc_queue_t();
    ~mpsc_queue_t();

    // Insert a value in the queue.
    void enqueue(const T& value);

    // Extract a value from the queue.
    // If the queue is empty, the value will be left unset.
    // The caller should properly initialize the value before this call,
    // to be able to determine if it was actually set.
    void dequeue(T& value);

    inline bool is_empty() const;

    inline size_t size() const;

protected:
    void enqueue_internal(mpsc_queue_node_t<T>* node);
    mpsc_queue_node_t<T>* dequeue_internal();

protected:
    std::atomic<mpsc_queue_node_t<T>*> m_head;
    mpsc_queue_node_t<T>* m_tail;

    // The queue structure is guaranteed to never actually be empty
    // by having it contain references to this "stub" node when it is logically empty.
    mpsc_queue_node_t<T> m_stub;

    std::atomic<size_t> m_size;
};

#include "queue.inc"

/*@}*/
} // namespace common
/*@}*/
} // namespace gaia
