/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "retail_assert.hpp"
#include "rule_thread_pool.hpp"
#include "auto_tx.hpp"
#include "event_manager.hpp"

#include <cstring>

using namespace gaia::rules;
using namespace gaia::common;
using namespace std;

/**
 * Thread local variable instances
 */
thread_local bool rule_thread_pool_t::s_tls_can_enqueue = true;
thread_local queue<rule_context_t> rule_thread_pool_t::s_tls_pending_invocations;


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

    for (uint32_t i = 0; i < num_threads; i++)
    {
        thread worker([this]{ rule_worker(); });
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
    if (m_num_threads > 0)
    {
        m_exit = true;
        m_invocations_signal.notify_all();
        for (thread& worker : m_threads)
        {
            worker.join();
        }
    }
}

void rule_thread_pool_t::execute_immediate()
{
    retail_assert(m_num_threads == 0, "Thread pool should have 0 workers for executing immediate!");

    // If s_tls_can_enqueue is false then this means that a rule
    // is in the middle of executing and issued a commit.  We have to wait
    // until the last commit is done, however, so keep queueing.  The top
    // level call to execute_immediate will drain the queue.  Since this
    // all happens on the same thread, it is safe (unless the empty() method
    // caches a value and we don't drain the queue).
    if (s_tls_can_enqueue)
    {
        while (!m_invocations.empty())
        {
            rule_context_t context = m_invocations.front();
            m_invocations.pop();
            invoke_rule(&context);
        }
    }
}

void rule_thread_pool_t::enqueue(rule_context_t& invocation)
{
    if (s_tls_can_enqueue)
    {
        if (m_num_threads > 0)
        {
            unique_lock<mutex> lock(m_lock);
            m_invocations.push(invocation);
            lock.unlock();
            m_invocations_signal.notify_one();
        }
        else
        {
            m_invocations.push(invocation);
        }
    }
    else
    {
        s_tls_pending_invocations.push(invocation);
    }
}

// Thread worker function.
void rule_thread_pool_t::rule_worker()
{
    unique_lock<mutex> lock(m_lock, defer_lock);
    begin_session();
    while (true)
    {
        lock.lock();
        m_invocations_signal.wait(lock, [this] {
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

        invoke_rule(&context);
    }
    end_session();
}

void rule_thread_pool_t::invoke_rule(const rule_context_t* context)
{
    s_tls_can_enqueue = false;
    bool should_schedule = false;
    try
    {
        gaia::db::begin_transaction();

        // Invoke the rule.
        context->rule_binding.rule(context);

        // The rule may have committed the thread transaction
        // so don't try to commit it again.
        if (gaia::db::is_transaction_active())
        {
            gaia::db::commit_transaction();
        }
        should_schedule = true;
    }
    catch(const std::exception& e)
    {
        if (gaia::db::is_transaction_active())
        {
            gaia::db::rollback_transaction();
        }
        // TODO[GAIAPLAT-129]: Log an error in an error table here.
        // TODO[GAIAPLAT-158]: Determine retry/error handling logic
        // Catch all exceptions or let terminate happen? Don't drop pending
        // rules on the floor (should_schedule == false) when we add retry logic.
    }
    s_tls_can_enqueue = true;
    process_pending_invocations(should_schedule);
}

void rule_thread_pool_t::process_pending_invocations(bool should_schedule)
{
    while(!s_tls_pending_invocations.empty())
    {
        if (should_schedule)
        {
            rule_context_t context = s_tls_pending_invocations.front();
            enqueue(context);
        }
        s_tls_pending_invocations.pop();
    }
}
