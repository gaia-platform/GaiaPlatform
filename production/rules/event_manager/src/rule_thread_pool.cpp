/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "retail_assert.hpp"
#include "rule_thread_pool.hpp"
#include "event_manager.hpp"

#include <cstring>

using namespace gaia::rules;
using namespace gaia::common;
using namespace std;

/**
 * Thread local variable instances
 */
thread_local bool rule_thread_pool_t::s_tls_can_enqueue = true;
thread_local queue<rule_thread_pool_t::invocation_t> rule_thread_pool_t::s_tls_pending_invocations;


void rule_thread_pool_t::log_events(invocation_t& invocation)
{
    auto& log_invocation = std::get<log_events_invocation_t>(invocation.args);
    retail_assert(log_invocation.events.size() == log_invocation.rules_invoked.size(), 
        "Event vector and rules_invoked vector sizes must match!");

    gaia::db::begin_transaction();
    {
        invocation.record_rule_stats([&]() {

        for (size_t i = 0; i < log_invocation.events.size(); i++)
        {
            uint64_t timestamp = (uint64_t)time(NULL);
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
                (uint32_t)(event.event_type), 
                (uint64_t)(event.gaia_type),
                (uint64_t)(event.record), 
                column_id, 
                timestamp, rule_invoked);
        }

        }); // recorder
    }
    gaia::db::commit_transaction();
}

rule_thread_pool_t::rule_thread_pool_t(size_t num_threads)
{
    m_exit = false;
    m_num_threads = (num_threads == SIZE_MAX) ? thread::hardware_concurrency() : num_threads; 

    for (uint32_t i = 0; i < m_num_threads; i++)
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
            invocation_t context = m_invocations.front();
            m_invocations.pop();

            context.record_invoke_stats([&]() {
                invoke_rule(context);
            });
        }
    }
}

void rule_thread_pool_t::enqueue(invocation_t& invocation)
{
    if (s_tls_can_enqueue)
    {
        if (m_num_threads > 0)
        {
            unique_lock<mutex> lock(m_lock);
            // Record the time after we have acquired the lock
            invocation.record_enqueue_stats();
            m_invocations.push(invocation);
            lock.unlock();
            m_invocations_signal.notify_one();
        }
        else
        {
            invocation.record_enqueue_stats();
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

        invocation_t context = m_invocations.front();
        m_invocations.pop();
        lock.unlock();

        context.record_invoke_stats([&]() {
            invoke_rule(context);
        });
        context.log_stats();
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

    try
    {
        auto_transaction_t transaction(auto_transaction_t::no_auto_begin);
        rule_context_t context(
            transaction,
            rule_invocation.gaia_type,
            rule_invocation.event_type,
            rule_invocation.record,
            rule_invocation.fields
        );

        // Invoke the rule.
        invocation.record_rule_stats([&]() {
            rule_invocation.rule_fn(&context);
        });

        should_schedule = true;
        s_tls_can_enqueue = true;
        if (gaia::db::is_transaction_active())
        {
            transaction.commit();
        }
    }
    catch(const std::exception& e)
    {
        // TODO[GAIAPLAT-129]: Log an error in an error table here.
        // TODO[GAIAPLAT-158]: Determine retry/error handling logic
        // Catch all exceptions or let terminate happen? Don't drop pending
        // rules on the floor (should_schedule == false) when we add retry logic.
    }

    process_pending_invocations(should_schedule);
}

void rule_thread_pool_t::process_pending_invocations(bool should_schedule)
{
    while(!s_tls_pending_invocations.empty())
    {
        if (should_schedule)
        {
            invocation_t invocation = s_tls_pending_invocations.front();
            enqueue(invocation);
        }
        s_tls_pending_invocations.pop();
    }
}

void rule_thread_pool_t::invocation_t::record_enqueue_stats()
{
    if (stats_ptr)
    {
        stats_ptr->enqueue_time = perf_timer_t::get_time_point();
    }
}

void rule_thread_pool_t::invocation_t::record_rule_stats(std::function<void ()> fn)
{
    if (stats_ptr)
    {
        stats_ptr->before_rule = perf_timer_t::get_time_point();
        fn();
        stats_ptr->after_rule = perf_timer_t::get_time_point();
    }
    else
    {
        fn();
    }
}

void rule_thread_pool_t::invocation_t::record_invoke_stats(std::function<void ()> fn)
{
    if (stats_ptr)
    {
        stats_ptr->before_invoke = perf_timer_t::get_time_point();
        fn();
        stats_ptr->after_invoke = perf_timer_t::get_time_point();
    }
    else
    {
        fn();
    }
}

void rule_thread_pool_t::invocation_t::init_stats(
    std::chrono::high_resolution_clock::time_point start_time)
{
    stats_ptr.reset(new rule_thread_pool_t::rule_stats_t());
    stats_ptr->start_time = start_time;
}

void rule_thread_pool_t::invocation_t::log_stats()
{
    if (!stats_ptr)
    {
        return;
    }

    //TODO[GAIAPLAT-318] Use an actual logging library when available.
    const char* tag = (type == invocation_type_t::log_events) ? "log_events" : "user";
    auto enq = stats_ptr->enqueue_time - stats_ptr->start_time;
    auto signal = stats_ptr->before_invoke - stats_ptr->enqueue_time;
    auto before = stats_ptr->before_rule - stats_ptr->before_invoke;
    auto rule = stats_ptr->after_rule - stats_ptr->before_rule;
    auto after = stats_ptr->after_invoke - stats_ptr->after_rule;
    auto total = stats_ptr->after_invoke - stats_ptr->start_time;

    auto result = std::chrono::duration_cast<std::chrono::nanoseconds>(enq).count();
    printf("[(%s) commit_trigger -> enqueue]:  %0.2f us\n", tag, perf_timer_t::ns_us(result));
    
    result = std::chrono::duration_cast<std::chrono::nanoseconds>(signal).count();
    printf("[(%s) enqueue -> dequeue]:  %0.2f us\n", tag, perf_timer_t::ns_us(result));

    result = std::chrono::duration_cast<std::chrono::nanoseconds>(before).count();
    printf("[(%s) dequeue -> before rule]:  %0.2f us\n", tag, perf_timer_t::ns_us(result));

    result = std::chrono::duration_cast<std::chrono::nanoseconds>(rule).count();
    printf("[(%s) before rule -> after rule]:  %0.2f us\n", tag, perf_timer_t::ns_us(result));

    result = std::chrono::duration_cast<std::chrono::nanoseconds>(after).count();
    printf("[(%s) after rule -> after invocation]:  %0.2f us\n", tag, perf_timer_t::ns_us(result));
    
    result = std::chrono::duration_cast<std::chrono::nanoseconds>(total).count();
    printf("[(%s) total]:  %0.2f us\n", tag, perf_timer_t::ns_us(result));
}
