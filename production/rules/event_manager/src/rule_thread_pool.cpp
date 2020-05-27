/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "retail_assert.hpp"
#include "rule_thread_pool.hpp"
#include "auto_tx.hpp"

#include <cstring>

using namespace gaia::rules;
using namespace gaia::common;
using namespace std;

rule_thread_pool_t::rule_thread_pool_t(uint32_t num_threads)
{
    init(num_threads);
}

rule_thread_pool_t::rule_thread_pool_t()
{
    init(thread::hardware_concurrency());
}

void rule_thread_pool_t::init(uint32_t num_threads)
{
    m_exit = false;
    m_num_threads = num_threads;

    for (size_t i = 0; i < num_threads; i++)
    {
        thread worker([this]{ this->invoke_rule();});
        m_threads.push_back(move(worker));
    }
}

uint32_t rule_thread_pool_t::get_num_threads()
{
    return m_num_threads;
}

// Shutdown all threads in the pool
rule_thread_pool_t::~rule_thread_pool_t()
{
    m_exit = true;
    m_has_invocations.notify_all();
    for (thread& worker : m_threads)
    {
        worker.join();
    }
}

void rule_thread_pool_t::enqueue(rule_context_t& invocation)
{
    unique_lock<mutex> lock(m_lock);
    m_invocations.push(invocation);
    lock.unlock();
    m_has_invocations.notify_one();
}

// thread worker function
void rule_thread_pool_t::invoke_rule()
{
    unique_lock<mutex> lock(m_lock, defer_lock);

    while (true)
    {
        lock.lock();
        m_has_invocations.wait(lock, [this] {
            return (m_invocations.size() > 0 || m_exit);
        });

        if (m_exit)
        {
            break;
        }

        // should move instead of copy this
        rule_context_t context = m_invocations.front();
        m_invocations.pop();
        lock.unlock();

        // Invoke the rule
        // UNDONE: handle errors
        // UNDONE: start transaction
        context.rule_binding.rule(&context);
    }
}