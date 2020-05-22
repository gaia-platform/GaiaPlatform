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

typedef void (* rule_thread_fn)();

class rule_thread_pool_t
{
public:
    // allocate resources here
    rule_thread_pool_t();
    ~rule_thread_pool_t();    

    // use move semantics here
    // std::move
    void enqueue(rule_context_t& invocation);

    // our thread worker
    void invoke_rule();

private:
    queue<rule_context_t> m_invocations;
    vector<thread> m_threads;
    mutex m_lock;
    condition_variable m_has_invocations;
    bool m_exit;
};

} // namespace rules
} // namespace gaia
