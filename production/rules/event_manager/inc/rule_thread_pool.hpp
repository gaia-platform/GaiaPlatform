/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "rules.hpp"

namespace gaia 
{
namespace rules
{


class rule_thread_pool_t
{
public:
    rule_thread_pool_t() = delete;

    /**
     * Construct a thread pool used for executing rules.
     * 
     * @param num_threads create a pool with this many worker threads.
     * If 0 threads are specified then the thread pool is in "immediate"
     * mode and no worker threads are created. If SIZE_MAX is specified
     * then create thepool with the number of available hardware threads.
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
     * @param invocation the context of the rule to invoke.  Note that the 
     *   context includes the rule function pointer itself.
     * @param immediate if True then the rule is invoked immediately instead of
     *   begin placed in the queue.
     */
    void enqueue(rule_context_t& invocation);

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
    void invoke_rule(const rule_context_t* context);
    void process_pending_invocations(bool should_schedule);

private:
    // Each thread has a copy of these two variables to determine
    // whether pending rule invocations can be scheduled or they
    // have to wait.  If they have to wait then it is the responsibility
    // of the thread they are waiting on to move them over.
    static thread_local bool s_tls_can_enqueue;
    static thread_local queue<rule_context_t> s_tls_pending_invocations;

    /**
     * Queue from which worker thread draw their invocations.
     */
    queue<rule_context_t> m_invocations;

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
