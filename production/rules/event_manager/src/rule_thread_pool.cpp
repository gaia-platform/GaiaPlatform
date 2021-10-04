/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "rule_thread_pool.hpp"

#include <cstring>

#include "gaia_internal/common/logger_internal.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/gaia_db_internal.hpp"

using namespace std;

using namespace gaia::rules;
using namespace gaia::common;
using namespace gaia::direct_access;

/**
 * Thread local variable instances
 */
thread_local bool rule_thread_pool_t::s_tls_can_enqueue = true;
thread_local queue<rule_thread_pool_t::invocation_t> rule_thread_pool_t::s_tls_pending_invocations;

rule_thread_pool_t::rule_thread_pool_t(size_t count_threads, uint32_t max_retries, rule_stats_manager_t& stats_manager, rule_checker_t& rule_checker)
    : m_stats_manager(stats_manager)
    , m_rule_checker(rule_checker)
    , m_max_rule_retries(max_retries)
    , m_count_busy_workers(count_threads)
{
    m_exit = false;
    for (uint32_t i = 0; i < count_threads; i++)
    {
        thread worker([this] { rule_worker(std::ref(m_count_busy_workers)); });
        m_threads.emplace_back(move(worker));
    }
}

size_t rule_thread_pool_t::get_num_threads()
{
    return m_threads.size();
}

rule_thread_pool_t::~rule_thread_pool_t()
{
    shutdown();
}

void rule_thread_pool_t::wait_for_rules_to_complete()
{
    unique_lock lock(m_lock, defer_lock);
    wait_for_rules_to_complete(lock);
    lock.unlock();
}

// NOTE:  Callers must unlock the passed in mutex.
//
// TODO[GAIAPLAT-1020]: Add a configuration setting to limit the time
// we will wait for all rules to execute.
void rule_thread_pool_t::wait_for_rules_to_complete(std::unique_lock<std::mutex>& lock)
{
    while (true)
    {
        lock.lock();
        ASSERT_INVARIANT(m_count_busy_workers >= 0, "Invalid state. Cannot have more busy workers than threads in the pool!");
        if (m_count_busy_workers == 0 && m_invocations.size() == 0)
        {
            break;
        }
        lock.unlock();
        std::this_thread::yield();
    }
}

void rule_thread_pool_t::shutdown()
{
    if (m_threads.size() == 0)
    {
        return;
    }

    auto start_shutdown_time = gaia::common::timer_t::get_time_point();

    unique_lock lock(m_lock, defer_lock);

    // Wait for the currently executing rules "graph" to finish executing.  This will
    // drain the work queues of any rules executing AND any rules that these rules
    // may trigger.
    wait_for_rules_to_complete(lock);

    m_exit = true;
    lock.unlock();
    m_invocations_signal.notify_all();

    for (thread& worker : m_threads)
    {
        worker.join();
    }
    m_threads.clear();

    int64_t shutdown_duration = gaia::common::timer_t::get_duration(start_shutdown_time);
    gaia_log::rules().trace("shutdown took {:.2f} ms", gaia::common::timer_t::ns_to_ms(shutdown_duration));
}

void rule_thread_pool_t::execute_immediate()
{
    ASSERT_PRECONDITION(m_threads.size() == 0, "Thread pool should have 0 workers for executing immediate!");

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
    m_stats_manager.insert_rule_stats(invocation.rule_id);

    if (s_tls_can_enqueue)
    {
        m_stats_manager.inc_scheduled(invocation.rule_id);
        if (m_threads.size() > 0)
        {
            unique_lock lock(m_lock);
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
        m_stats_manager.inc_pending(invocation.rule_id);
        s_tls_pending_invocations.push(invocation);
    }
}

// Thread worker function.
void rule_thread_pool_t::rule_worker(int32_t& count_busy_workers)
{
    unique_lock lock(m_lock, defer_lock);

    db::begin_session();
    while (true)
    {
        lock.lock();
        --count_busy_workers;
        m_invocations_signal.wait(lock, [this] { return (m_invocations.size() > 0 || m_exit); });
        ++count_busy_workers;

        if (m_exit)
        {
            lock.unlock();
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
    db::end_session();
}

// We must worry about user-rules that throw exceptions, end the transaction
// started by the rules engine, and log the event.
void rule_thread_pool_t::invoke_rule(invocation_t& invocation)
{
    if (invocation.serial_group == nullptr)
    {
        invoke_rule_inner(invocation);
        return;
    }

    unique_lock execute_lock{invocation.serial_group->execute_lock, defer_lock};
    if (execute_lock.try_lock())
    {
        invoke_rule_inner(invocation);
    }
    else
    {
        unique_lock enqueue_lock{invocation.serial_group->enqueue_lock};
        invocation.serial_group->invocations.push(invocation);
        execute_lock.try_lock();
    }

    if (execute_lock)
    {
        while (true)
        {
            unique_lock enqueue_lock{invocation.serial_group->enqueue_lock};
            if (invocation.serial_group->invocations.empty())
            {
                break;
            }
            invocation_t new_invocation = invocation.serial_group->invocations.front();
            invocation.serial_group->invocations.pop();
            enqueue_lock.unlock();

            invoke_rule_inner(new_invocation);
        }
    }
}

void rule_thread_pool_t::invoke_rule_inner(invocation_t& invocation)
{
    rule_invocation_t& rule_invocation = invocation.args;
    s_tls_can_enqueue = false;
    bool should_schedule = false;
    const char* rule_id = invocation.rule_id;

    m_stats_manager.inc_executed(rule_id);
    try
    {
        try
        {
            auto_transaction_t txn(auto_transaction_t::no_auto_begin);

            // If the anchor row is invalid, then do not invoke the rule.  This can
            // occur if the row is deleted after it has been inserted or updated but
            // before an enqueued rule has been invoked.
            if (!m_rule_checker.is_valid_row(rule_invocation.record))
            {
                gaia_log::rules().trace("invalid anchor row: rule '{}' was not invoked, src_txn:'{}', new_txn:'{}'", rule_id, rule_invocation.src_txn_id, gaia::db::get_txn_id());

                // It is safe to exit early out of this routine. The transaction will clean up on
                // exit of the function and there will be no pending rule invocations to process.
                // An invocation can only be pending if the rule itself called commit such that
                // a new rule is enqueued while the existing rule is executing.  Since we
                // never called a rule function, this cannot happen.
                ASSERT_INVARIANT(s_tls_pending_invocations.empty(), "No pending invocations should exist!");
                return;
            }

            rule_context_t context(
                txn,
                rule_invocation.gaia_type,
                rule_invocation.event_type,
                rule_invocation.record);

            m_stats_manager.compute_rule_invocation_latency(rule_id, invocation.start_time);

            // Invoke the rule.
            auto fn_start = gaia::common::timer_t::get_time_point();
            gaia_log::rules().trace("call:'{}', src_txn:'{}', new_txn:'{}'", rule_id, rule_invocation.src_txn_id, gaia::db::get_txn_id());
            rule_invocation.rule_fn(&context);
            gaia_log::rules().trace("return:'{}'", rule_id);
            m_stats_manager.compute_rule_execution_time(rule_id, fn_start);

            should_schedule = true;
            s_tls_can_enqueue = true;
            if (gaia::db::is_transaction_open())
            {
                txn.commit();
            }
        }
        catch (const gaia::db::transaction_update_conflict& e)
        {
            // If should_schedule == false, rule scheduling failed and we drop any pending
            // invocations. We may retry our current rule and re-enqueue our pending.
            should_schedule = false;
            if (invocation.num_retries >= m_max_rule_retries)
            {
                gaia_log::rules().error("serialization exception: '{}'", rule_id);
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
    catch (const gaia::common::retail_assertion_failure& e)
    {
        // Always rethrow internal asserts.  Do not eat them and
        // do not pass them along to the user's exception handler.
        // TODO[GAIAPLAT-446]: Before shipping V1, we need to review
        // all retail asserts to ensure that they always indicate
        // an internal error.
        throw;
    }
    catch (const std::exception& e)
    {
        m_stats_manager.inc_exceptions(rule_id);
        gaia_log::rules().warn("exception: '{}', '{}'", rule_id, e.what());
        gaia::rules::handle_rule_exception();
    }
    catch (...)
    {
        m_stats_manager.inc_exceptions(rule_id);
        gaia_log::rules().warn("exception: '{}', '{}'", rule_id, "Unknown exception.");
        gaia::rules::handle_rule_exception();
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
