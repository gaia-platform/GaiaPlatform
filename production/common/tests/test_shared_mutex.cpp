/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <pthread.h>

#include <iostream>

#include "gtest/gtest.h"

#include "gaia_synchronization.hpp"

using namespace std;
using namespace gaia::common;

struct context_t
{
    shared_mutex lock;

    int count_reader_executions;
    int count_writer_executions;
    int count_readers_acquiring_lock;
    int count_writers_acquiring_lock;

    context_t()
    {
        reset();
    }

    void reset()
    {
        count_reader_executions = 0;
        count_writer_executions = 0;

        count_readers_acquiring_lock = 0;
        count_writers_acquiring_lock = 0;
    }
};

void* reader(void* argument)
{
    context_t* context = static_cast<context_t*>(argument);

    if (context->lock.try_lock_shared())
    {
        context->count_readers_acquiring_lock++;

        context->lock.unlock_shared();
    }

    context->count_reader_executions++;

    return nullptr;
}

void* writer(void* argument)
{
    context_t* context = static_cast<context_t*>(argument);

    if (context->lock.try_lock())
    {
        context->count_writers_acquiring_lock++;

        context->lock.unlock();
    }

    context->count_writer_executions++;

    return nullptr;
}

void write_after_operation(bool is_read_operation)
{
    context_t context;
    pthread_t tid;
    int error_code;

    // Acquire lock.
    is_read_operation ? context.lock.lock_shared() : context.lock.lock();

    // Spawn writer.
    error_code = pthread_create(&tid, nullptr, &writer, &context);
    ASSERT_EQ(0, error_code);

    // Wait for all threads to complete execution.
    while (context.count_writer_executions < 1)
    {
        usleep(1);
    }

    // Release lock.
    is_read_operation ? context.lock.unlock_shared() : context.lock.unlock();

    ASSERT_EQ(0, context.count_writers_acquiring_lock);

    // Spawn a new writer.
    error_code = pthread_create(&tid, nullptr, &writer, &context);
    ASSERT_EQ(0, error_code);

    // Wait for all threads to complete execution.
    while (context.count_writer_executions < 2)
    {
        usleep(1);
    }

    ASSERT_EQ(0, context.count_readers_acquiring_lock);
    ASSERT_EQ(1, context.count_writers_acquiring_lock);
}

TEST(common, shared_mutex_write_after_read)
{
    write_after_operation(true);
}

TEST(common, shared_mutex_write_after_write)
{
    write_after_operation(false);
}

TEST(common, shared_mutex_read_after_write)
{
    context_t context;
    pthread_t tid;
    int error_code;

    // Take a write lock.
    context.lock.lock();

    // Spawn reader.
    error_code = pthread_create(&tid, nullptr, &reader, &context);
    ASSERT_EQ(0, error_code);

    // Wait for all threads to complete execution.
    while (context.count_reader_executions < 1)
    {
        usleep(1);
    }

    // Release lock.
    context.lock.unlock();

    ASSERT_EQ(0, context.count_readers_acquiring_lock);

    // Spawn a new reader.
    error_code = pthread_create(&tid, nullptr, &reader, &context);
    ASSERT_EQ(0, error_code);

    // Wait for all threads to complete execution.
    while (context.count_reader_executions < 2)
    {
        usleep(1);
    }

    ASSERT_EQ(1, context.count_readers_acquiring_lock);
    ASSERT_EQ(0, context.count_writers_acquiring_lock);
}

TEST(common, shared_mutex_read_after_read)
{
    context_t context;
    pthread_t tid;
    int error_code;

    // Take a read lock.
    context.lock.lock_shared();

    // Spawn reader.
    error_code = pthread_create(&tid, nullptr, &reader, &context);
    ASSERT_EQ(0, error_code);

    // Wait for all threads to complete execution.
    while (context.count_reader_executions < 1)
    {
        usleep(1);
    }

    // Release lock.
    context.lock.unlock_shared();

    ASSERT_EQ(1, context.count_readers_acquiring_lock);
    ASSERT_EQ(0, context.count_writers_acquiring_lock);
}
