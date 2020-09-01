/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "rule_stats_manager.hpp"

using namespace gaia::common;
using namespace gaia::rules;
using namespace std;


bool rule_stats_manager_t::s_enabled = false;
const char* rule_stats_manager_t::s_log_event_tag = "log_event_rule";
const char* rule_stats_manager_t::s_rule_tag = "user_rule";

void rule_stats_manager_t::log(shared_ptr<rule_stats_t>& stats_ptr)
{
    if (!s_enabled)
    {
        return;
    }

    //TODO[GAIAPLAT-318] Use an actual logging library when available.
    auto enq = stats_ptr->enqueue_time - stats_ptr->start_time;
    auto signal = stats_ptr->before_invoke - stats_ptr->enqueue_time;
    auto before = stats_ptr->before_rule - stats_ptr->before_invoke;
    auto rule = stats_ptr->after_rule - stats_ptr->before_rule;
    auto after = stats_ptr->after_invoke - stats_ptr->after_rule;
    auto total = stats_ptr->after_invoke - stats_ptr->start_time;

    auto result = std::chrono::duration_cast<std::chrono::nanoseconds>(enq).count();
    printf("[(%s) commit_trigger -> enqueue]:  %0.2f us\n", stats_ptr->tag, perf_timer_t::ns_us(result));
    
    result = std::chrono::duration_cast<std::chrono::nanoseconds>(signal).count();
    printf("[(%s) enqueue -> dequeue]:  %0.2f us\n", stats_ptr->tag, perf_timer_t::ns_us(result));

    result = std::chrono::duration_cast<std::chrono::nanoseconds>(before).count();
    printf("[(%s) dequeue -> before rule]:  %0.2f us\n", stats_ptr->tag, perf_timer_t::ns_us(result));

    result = std::chrono::duration_cast<std::chrono::nanoseconds>(rule).count();
    printf("[(%s) before rule -> after rule]:  %0.2f us\n", stats_ptr->tag, perf_timer_t::ns_us(result));

    result = std::chrono::duration_cast<std::chrono::nanoseconds>(after).count();
    printf("[(%s) after rule -> after invocation]:  %0.2f us\n", stats_ptr->tag, perf_timer_t::ns_us(result));
    
    result = std::chrono::duration_cast<std::chrono::nanoseconds>(total).count();
    printf("[(%s) total]:  %0.2f us\n", stats_ptr->tag, perf_timer_t::ns_us(result));
}
