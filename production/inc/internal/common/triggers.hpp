/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "events.hpp"
#include <stdint.h>
#include <vector>
#include <memory>
#include <atomic>
#include <functional>

#include "wait_queue.hpp"
#include "gaia_common.hpp"

using namespace gaia::common;

namespace gaia 
{
namespace db
{
namespace triggers {

struct trigger_event_t {
    event_type_t event_type; // insert, update, delete, begin, commit, rollback
    gaia_id_t gaia_type; // gaia table type, maybe 0 if event has no associated tables
    gaia_id_t record; //row id, may be 0 if if there is no assocated row id
    const u_int16_t* columns; // list of affected columns, may be null
    size_t count_columns; // count of affected columns, may be zero
};

/**
 * The type of Gaia commit trigger.
 */
typedef void (*f_commit_trigger_t) (uint64_t, std::vector<trigger_event_t>, bool);



// /**
//  * This file implements a thread safe lock based queue.
//  * Todo (msj) Make this threadpool generic to handle waitable tasks too.
//  */ 
// class event_trigger_threadpool {
//     private:
//         // Rules engine needs access to this variable.
//         static f_commit_trigger_t s_tx_commit_trigger;
        
//         // Flag serves as a way to initialize thread resources. 
//         thread_local static std::atomic_bool session_active;

//         std::atomic_bool has_execution_completed;
//         wait_queue<std::function<void()>> tasks;
//         std::vector<std::thread> workers;

//         void run_method() {

//             // Initialize resources.
//             if (!session_active) {
//                 init_thread_resources();
//                 session_active = true;
//             }

//             if (session_active && !has_execution_completed) {
//                 std::function<void()> task;
//                 tasks.pop(task);
//                 task();
//             }

//             if (session_active && has_execution_completed) {
//                 cleanup_thread_resources();
//             }

//         }
 
//         public: 
//             event_trigger_threadpool() {
//                 has_execution_completed = false;
//                 auto count = 1;
//                 for (int i = 0; i < count; i++) {
//                     workers.push_back(std::thread(&event_trigger_threadpool::run_method, this));
//                 }
//             }

//             ~event_trigger_threadpool() {
//                 has_execution_completed = true;
//                 for (auto& worker: workers) {
//                     worker.join();
//                 }
//             }

//             /**
//             * Invoked by the Rules Engine on system initialization. 
//             */ 
//             static void set_commit_trigger(f_commit_trigger_t commit_trigger) {
//                 s_tx_commit_trigger = commit_trigger;
//             }

//             /**
//              * Invoked by the Storage Engine during transaction commit to check whether the rules engine has
//              * been initialized. We don't need synchronization here as the rules engine will be activated on startup, 
//              * whereas transaction commit occurs much later.
//              */
//             static f_commit_trigger_t get_commit_trigger() {
//                 return s_tx_commit_trigger;
//             }

//             void add_trigger_task(gaia_xid_t transaction_id, std::vector<triggers::trigger_event_t> events) {
//                 // Pass vector of events by copy to capture for now.
//                 // We can avoid the copy with a custom allocator (if copying becomes a problem)
//                 // Todo - deep copy events post yi wen's changes?
//                 tasks.push([=] () {
//                     s_tx_commit_trigger(transaction_id, std::move(events), true);
//                 });
//             }

//             /**
//              * These can be overridden to add custom logic to the threadpool threads.
//              * In the current use case: we need to perform a begin_session() before begin_transaction()
//              * And similarly, end_session() should be called when the thread terminates.
//              */ 
//             void init_thread_resources() { };
//             void cleanup_thread_resources() { };
        

//     };
}
}
}
