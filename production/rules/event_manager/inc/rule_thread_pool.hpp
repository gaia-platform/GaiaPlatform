/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "gaia_event_log.h"
#include "triggers.hpp"
#include "rules.hpp"
#include "rule_stats_manager.hpp"

#include <variant>

namespace gaia 
{
namespace rules
{


class rule_thread_pool_t
{
public:

    enum class invocation_type_t : uint8_t 
    {
        rule = 0,
        log_events = 1
    };

   struct log_events_invocation_t {
       const db::triggers::trigger_event_list_t events;
       const vector<bool> rules_invoked;
   };

   struct rule_invocation_t {
        gaia_rule_fn rule_fn;
        common::gaia_type_t gaia_type;
        db::triggers::event_type_t event_type;
        gaia_id_t record;
        field_position_list_t fields;
   };

    struct invocation_t {
        invocation_type_t type;
        std::variant<rule_invocation_t, log_events_invocation_t> args;
        shared_ptr<rule_stats_t> stats;
    };

    /**
     * System rules.  Currently the only system function we support
     * is logging to the event table.
     */
    static void log_events(invocation_t& invocation);

    rule_thread_pool_t() = delete;

    /**
     * Construct a thread pool used for executing rules.
     * 
     * @param num_threads create a pool with this many worker threads.
     * If 0 threads are specified then the thread pool is in "immediate"
     * mode and no worker threads are created. If SIZE_MAX is specified
     * then create the pool with the number of available hardware threads.
     */ 
    rule_thread_pool_t(size_t num_threads);

    /**
     * Will notify and wait for all workers in the thread pool
     * to finish executing their last work item before destroying
     * the pool
     */
    ~rule_thread_pool_t();

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
    uint32_t get_num_threads();

private:
    void rule_worker();

    void inline invoke_rule(invocation_t& invocation) 
    {
        if (invocation_type_t::rule == invocation.type)
        {
            invoke_user_rule(invocation);
        }
        else
        {
            log_events(invocation);
        }
    }

    void invoke_user_rule(invocation_t& invocation);
    void process_pending_invocations(bool should_schedule);

    // Each thread has a copy of these two variables to determine
    // whether pending rule invocations can be scheduled or they
    // have to wait.  If they have to wait then it is the responsibility
    // of the thread they are waiting on to move them over.
    static thread_local bool s_tls_can_enqueue;
    static thread_local queue<invocation_t> s_tls_pending_invocations;

    /**
     * Queue from which worker thread draw their invocations.
     */
    queue<invocation_t> m_invocations;

    /**
     * OS threads waiting to do work
     */ 
    vector<thread> m_threads;

    /**
     * This lock together with the condition variable ensure synchronized 
     * access and signaling between the enqueue function and the worker
     * threads.
     */ 
    mutex m_lock;
    condition_variable m_invocations_signal;
    bool m_exit;
    size_t m_num_threads;
};

} // namespace rules
} // namespace gaia
