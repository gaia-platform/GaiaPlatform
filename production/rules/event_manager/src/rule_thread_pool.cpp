/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "rule_thread_pool.hpp"

#include <cstring>

#include "event_manager.hpp"
#include "logger.hpp"
#include "retail_assert.hpp"

using namespace std;

using namespace gaia::rules;
using namespace gaia::common;
using namespace gaia::direct_access;

/**
 * Thread local variable instances
 */
thread_local bool rule_thread_pool_t::s_tls_can_enqueue = true;
thread_local queue<rule_thread_pool_t::invocation_t> rule_thread_pool_t::s_tls_pending_invocations;

void rule_thread_pool_t::log_events(invocation_t& invocation)
{
    auto& log_invocation = std::get<log_events_invocation_t>(invocation.args);
    retail_assert(
        log_invocation.events.size() == log_invocation.rules_invoked.size(),
        "Event vector and rules_invoked vector sizes must match!");

    gaia::db::begin_transaction();
    {
        for (size_t i = 0; i < log_invocation.events.size(); i++)
        {
            auto timestamp = static_cast<uint64_t>(time(nullptr));
            uint16_t column_id = 0;
            auto& event = log_invocation.events[i];
            auto rule_invoked = log_invocation.rules_invoked[i];

            // TODO[GAIAPLAT-293]: add support for arrys of simple types
            // When we have this support we can support the array of changed column fields
            // in our event log.  Until then, just pick out the first of the list.
            if (event.columns.size() > 0)
            {
                column_id = event.columns[0];
            }

            event_log::event_log_t::insert_row(
                static_cast<uint32_t>(event.event_type),
                static_cast<uint64_t>(event.gaia_type),
                static_cast<uint64_t>(event.record),
                column_id,
                timestamp,
                rule_invoked);
        }
    }
    gaia::db::commit_transaction();
}

rule_thread_pool_t::rule_thread_pool_t(size_t count_threads, uint32_t max_retries, rule_stats_manager_t& stats_manager)
    : m_stats_manager(stats_manager), m_max_rule_retries(max_retries)
{
    m_exit = false;
    for (uint32_t i = 0; i < count_threads; i++)
    {
        thread worker([this] { rule_worker(); });
        m_threads.emplace_back(move(worker));
    }
}

size_t rule_thread_pool_t::get_num_threads()
{
    return m_threads.size();
}

// Shutdown all threads in the pool
rule_thread_pool_t::~rule_thread_pool_t()
{
    if (m_threads.size() > 0)
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
    retail_assert(m_threads.size() == 0, "Thread pool should have 0 workers for executing immediate!");

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
            auto start_thread_execution_time = gaia::common::timer_t::get_time_point();
            invocation_t invocation = m_invocations.front();
            m_invocations.pop();
            invoke_rule(invocation);
            m_stats_manager.compute_thread_execution_time(start_thread_execution_time);
        }
    }
}

void rule_thread_pool_t::enqueue(invocation_t& invocation)
{
    if (invocation.type == invocation_type_t::rule)
    {
        m_stats_manager.insert_rule_stats(invocation.rule_id);
        if (s_tls_can_enqueue)
        {
            m_stats_manager.inc_scheduled(invocation.rule_id);
        }
        else
        {
            m_stats_manager.inc_pending(invocation.rule_id);
        }
    }

    if (s_tls_can_enqueue)
    {
        if (m_threads.size() > 0)
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
        m_invocations_signal.wait(lock, [this] { return (m_invocations.size() > 0 || m_exit); });

        if (m_exit)
        {
            break;
        }

        // Calculate the amount of time spent doing actual work
        // in this thread.
        auto start_thread_execution_time = gaia::common::timer_t::get_time_point();
        invocation_t invocation = m_invocations.front();
        m_invocations.pop();
        lock.unlock();
        invoke_rule(invocation);
        m_stats_manager.compute_thread_execution_time(start_thread_execution_time);
    }
    end_session();
}

// We must worry about user-rules that throw exceptions, end the transaction
// started by the rules engine, and log the event.
void rule_thread_pool_t::invoke_user_rule(invocation_t& invocation)
{
    auto& rule_invocation = std::get<rule_invocation_t>(invocation.args);
    s_tls_can_enqueue = false;
    bool should_schedule = false;
    const char* rule_id = invocation.rule_id;

    try
    {
        try
        {
            auto_transaction_t txn(auto_transaction_t::no_auto_begin);
            rule_context_t context(
                txn,
                rule_invocation.gaia_type,
                rule_invocation.event_type,
                rule_invocation.record,
                rule_invocation.fields);

            m_stats_manager.compute_rule_invocation_latency(rule_id, invocation.start_time);

            // Invoke the rule.
            auto fn_start = gaia::common::timer_t::get_time_point();
            gaia_log::rules().trace("call: {}", rule_id);
            rule_invocation.rule_fn(&context);
            gaia_log::rules().trace("return: {}", rule_id);
            m_stats_manager.compute_rule_execution_time(rule_id, fn_start);

            should_schedule = true;
            s_tls_can_enqueue = true;
            if (gaia::db::is_transaction_active())
            {
                txn.commit();
            }
        }
        catch (const transaction_update_conflict& e)
        {
            // If should_schedule == false, rule scheduling failed and we drop any pending
            // invocations. We may retry our current rule and re-enqueue our pending.
            should_schedule = false;
            if (invocation.num_retries >= m_max_rule_retries)
            {
                throw;
            }
            else
            {
                invocation.num_retries++;
                m_stats_manager.inc_retries(rule_id);
                s_tls_can_enqueue = true;
                enqueue(invocation);
            }
        }
    }
    catch (const std::exception& e)
    {
        m_stats_manager.inc_exceptions(rule_id);
        gaia_log::rules().warn("exception: {}, {}", rule_id, e.what());
    }

    process_pending_invocations(should_schedule);
}

void rule_thread_pool_t::process_pending_invocations(bool should_schedule)
{
    while (!s_tls_pending_invocations.empty())
    {
        invocation_t invocation = s_tls_pending_invocations.front();
        if (should_schedule)
        {
            enqueue(invocation);
        }
        else
        {
            m_stats_manager.inc_abandoned(invocation.rule_id);
        }
        s_tls_pending_invocations.pop();
    }
}
