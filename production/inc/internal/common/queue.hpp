/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <unistd.h>

#include <synchronization.hpp>

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

template <class T> struct queue_element_t
{
    T value;

    queue_element_t* next;
    queue_element_t* previous;

    shared_mutex_t lock;

    queue_element_t() = default;
    queue_element_t(T value);
};

template <class T> class queue_t
{
public:

    queue_t();
    ~queue_t();

    void enqueue(T value);
    void dequeue(T& value);

    bool is_empty();

protected:

    queue_element_t<T> m_head;
    queue_element_t<T> m_tail;

    bool dequeue_internal(T& value);
};

#include "queue.inc"

/*@}*/
}
/*@}*/
}
