/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

#include "rules.hpp"

namespace gaia 
{
namespace rules
{


class rule_thread_pool_t
{
public:
    /**
     * Construct a thread pool used for executing rules.
     * 
     * @param num_threads create a pool with this many worker threads
     */ 
    rule_thread_pool_t(uint32_t num_threads);


    /**
     * Construct a thread pool with the number of hardware therads
     */
    rule_thread_pool_t();

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
     *  context includes the rule function pointer itself.
     */ 
    void enqueue(rule_context_t& invocation);

    /**
     * Returns the number of worker threads this thread pool is managing.
     * 
     * @return number of threads
     */
    uint32_t get_num_threads();

private:
    // our thread worker function
    typedef void (* rule_thread_fn)();
    void invoke_rule();

    void init(uint32_t num_threads);


    /**
     * queue from which worker thread draw their invocations
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
    condition_variable m_has_invocations;
    bool m_exit;
    uint32_t m_num_threads;
};

} // namespace rules
} // namespace gaia
