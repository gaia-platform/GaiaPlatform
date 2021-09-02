/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <condition_variable>

#include <mutex>
#include <queue>
#include <thread>
#include <variant>

#include "gaia/rules/rules.hpp"

#include "gaia_internal/db/triggers.hpp"

#include "rule_checker.hpp"
#include "rule_stats_manager.hpp"

namespace gaia
{
namespace rules
{

class rule_thread_pool_t
{
public:
    struct rule_invocation_t
    {
        gaia_rule_fn rule_fn;
        common::gaia_type_t gaia_type;
        db::triggers::event_type_t event_type;
        common::gaia_id_t record;
        common::field_position_list_t fields;
        db::gaia_txn_id_t src_txn_id;
    };

    struct serial_group_t;
    struct invocation_t
    {
        rule_invocation_t args;
        const char* rule_id;
        std::chrono::steady_clock::time_point start_time;
        std::shared_ptr<serial_group_t> serial_group{nullptr};
        uint32_t num_retries{0};
    };

    struct serial_group_t
    {
        std::mutex execute_lock;
        std::mutex enqueue_lock;
        std::queue<invocation_t> invocations;
    };

    /**
     * System rules.  Currently the only system function we support
     * is logging to the event table.
     */
    static void log_events(invocation_t& invocation);

    rule_thread_pool_t() = delete;
    ~rule_thread_pool_t();

    /**
     * Construct a thread pool used for executing rules.
     * 
     * @param num_threads create a pool with this many worker threads.
     * If 0 threads are specified then the thread pool is in "immediate"
     * mode and no worker threads are created. If SIZE_MAX is specified
     * then create the pool with the number of available hardware threads.
     */
    rule_thread_pool_t(
        size_t num_threads, uint32_t max_rule_retries,
        rule_stats_manager_t& stats_manager, rule_checker_t& rule_checker);

    /**
     * Wait for the current rules "graph" to execute. Wait for all rules to finish
     * executing as well as any rules that these rules enqueue.
     */
    void wait_for_rules_to_complete();

    /**
     * Notify and wait for all workers in the thread pool
     * to finish executing their last work item before destroying
     * the pool
     */
    void shutdown();

    /**
     * Enqueue a rule onto the thread pool and notify any worker thread
     * that a rule is ready to be invoked.
     * 
     * @param invocation the function pointer of the rule along with the
     *   trigger event information needed to call the rule.
     */
    void enqueue(invocation_t& invocation);

    /**
     * Executes all rules in the queue.  This method can only be called
     * if the thread pool was initialized with 0 threads
     */
    void execute_immediate();

    /**
     * Returns the number of worker threads this thread pool is managing.
     * 
     * @return number of threads
     */
    size_t get_num_threads();

private:
    void rule_worker(int32_t& count_busy_workers);
    void invoke_rule(invocation_t& invocation);
    void invoke_rule_inner(invocation_t& invocation);
    void process_pending_invocations(bool should_schedule);
    void wait_for_rules_to_complete(std::unique_lock<std::mutex>& lock);

    // Each thread has a copy of these two variables to determine
    // whether pending rule invocations can be scheduled or they
    // have to wait.  If they have to wait then it is the responsibility
    // of the thread they are waiting on to move them over.
    static thread_local bool s_tls_can_enqueue;
    static thread_local std::queue<invocation_t> s_tls_pending_invocations;

    /**
     * Queue from which worker thread draw their invocations.
     */
    std::queue<invocation_t> m_invocations;

    /**
     * Helper to calculate and log statistics for
     * the rules engine scheduler and each individual
     * user rule if desired.
     */
    rule_stats_manager_t& m_stats_manager;

    /**
     * Helper to validate anchor rows.
     */
    rule_checker_t& m_rule_checker;

    /**
     * Maximum number of times to retry a rule when getting transaction update conflicts.
     */
    const uint32_t m_max_rule_retries;

    /**
     * OS threads waiting to do work
     */
    std::vector<std::thread> m_threads;

    /**
     * This lock together with the condition variable ensure synchronized 
     * access and signaling between the enqueue function and the worker
     * threads.
     */
    std::mutex m_lock;
    std::condition_variable m_invocations_signal;
    int32_t m_count_busy_workers;
    bool m_exit;
};

} // namespace rules
} // namespace gaia
