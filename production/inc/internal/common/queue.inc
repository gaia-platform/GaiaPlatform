/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

template <class T>
queue_element_t<T>::queue_element_t()
    : value{}
{
}

template <class T>
queue_element_t<T>::queue_element_t(T value)
{
    this->value = value;

    next = nullptr;
    previous = nullptr;
}

template <class T>
queue_t<T>::queue_t()
{
    m_head.next = &m_tail;
    m_head.previous = nullptr;

    m_tail.next = nullptr;
    m_tail.previous = &m_head;
}

template <class T>
queue_t<T>::~queue_t()
{
    // Walk through the queue elements and deallocate them.
    queue_element_t<T>* walker = m_head.next;
    while (walker != &m_tail)
    {
        queue_element_t<T>* current = walker;
        walker = walker->next;
        delete current;
    }
}

template <class T>
void queue_t<T>::enqueue(const T& value)
{
    // Create element to hold new value.
    queue_element_t<T>* new_first_element = new queue_element_t<T>(value);

    // Link it to the head of the queue - this is safe to do.
    new_first_element->previous = &m_head;

    // For next operations, we need to take an exclusive lock on the head.
    std::unique_lock<std::shared_mutex> unique_lock_head(m_head.lock);

    queue_element_t<T>* current_first_element = m_head.next;

    // We also need to take a lock on the current first element,
    // to protect ourselves from a dequeue() operation that may try to concurrently remove it.
    // A shared lock will be sufficient here, as it will prevent the exclusive lock
    // requested by dequeue().
    std::shared_lock<std::shared_mutex> shared_lock_first(current_first_element->lock);

    // We can now safely insert our new node.
    new_first_element->next = current_first_element;
    m_head.next = new_first_element;
    current_first_element->previous = new_first_element;
}

template <class T>
void queue_t<T>::dequeue(T& value)
{
    while (!dequeue_internal(value))
    {
        // Keep trying. We'll either dequeue a value successfully,
        // or we'll terminate by finding the queue to be empty.
    }
}

template <class T>
bool queue_t<T>::dequeue_internal(T& value)
{
    queue_element_t<T>* current_last_element = nullptr;

    // Create scope for lock release.
    {
        // We need to take an exclusive lock on the tail.
        std::unique_lock<std::shared_mutex> unique_lock_tail(m_tail.lock);

        // Read the last element.
        queue_element_t<T>* current_last_element = m_tail.previous;

        // If the queue is empty, we're done, but first we need to release our lock.
        //
        // Note that this will prevent deadlocks: dequeue() will not hold a lock on an empty queue,
        // so enqueue() should always succeed in getting its second lock.
        if (current_last_element == &m_head)
        {
            return true;
        }

        // Before we process the current_last_element, we need to take an exclusive lock on it as well
        // This may make us wait behind another concurrent enqueue() operation that is trying to insert
        // before this node; however, that insertion will not impact our own operation.
        //
        // There is a second case in which we acquire this lock before the other enqueue() thread
        // and it is us that are blocking the other thread; that case is handled by the checks below.
        std::unique_lock<std::shared_mutex> unique_lock_last(current_last_element->lock);

        // With our second lock, it is safe to read the new last element.
        queue_element_t<T>* new_last_element = current_last_element->previous;

        // However, we still need a lock on this element,
        // in case this was the first element in the queue
        // and a concurrent enqueue() operation is attempting to use it.
        //
        // A shared lock will handle better the special case of concurrent enqueue()/dequeue() operations
        // on a list of size 2:
        // - enqueue() will acquire an exclusive lock on the head and a shared lock on the first element.
        // - dequeue() will acquire an exclusive lock on the head and the second element
        //   and a shared lock on the first element.
        // The concurrent operations on the first element are safe because they don't conflict:
        // enqueue() updates the previous link, whereas dequeue() updates the next link.
        std::shared_lock<std::shared_mutex> shared_lock_new_last(new_last_element->lock, std::defer_lock);
        if (shared_lock_new_last.try_lock() == false)
        {
            retail_assert(
                new_last_element == &m_head,
                "We should only fail acquiring a shared lock if we were asking for it for the head element.");

            // We've detected a concurrent enqueue() operation.
            // It is not safe to continue and remove the node before which the insertion should take place,
            // because its deallocation may cause the other thread to access invalid memory.
            // So we should abort our operation and have the caller retry it,
            // which will give a chance to the insertion to complete.
            // Release the locks and exit.
            return false;
        }

        // Otherwise we can extract the value from the last element.
        value = current_last_element->value;

        // Remove the old last element.
        new_last_element->next = &m_tail;
        m_tail.previous = new_last_element;
    }

    // Finally, we can deallocate the no longer needed element.
    delete current_last_element;

    return true;
}

template <class T>
bool queue_t<T>::is_empty() const
{
    return (m_tail.previous == &m_head);
}
