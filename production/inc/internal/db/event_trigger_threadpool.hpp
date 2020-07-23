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
#include "triggers.hpp"
#include "gaia_db.hpp"

using namespace gaia::common;

namespace gaia 
{
namespace db
{
namespace triggers {

class session_destructor {
    public:
        session_destructor() = default;
        ~session_destructor() {
            gaia::db::end_session();
        }
};

/**
 * Threadpool used to enque trigger event execution callbacks to the Rules Engine.
 * Todo (msj) Make this threadpool generic to handle waitable tasks too.
 */ 
class event_trigger_threadpool {
    private:
        // Rules engine needs access to this variable.
        static f_commit_trigger_t s_tx_commit_trigger;
        
        // Flag serves as a way to create a server session.
        thread_local static bool session_active;

        // Used to clean up state on thread termination.
        thread_local static session_destructor destroy_session;

        std::atomic_bool has_execution_completed;
        wait_queue<std::function<void()>> tasks;
        std::vector<std::thread> workers;

        void run_method() {

            if (!has_execution_completed) {
                std::function<void()> task;
                tasks.pop(task);
                // Before executing a task, create a session if it doesn't exist.
                if (!session_active) {
                    gaia::db::begin_session();
                    session_active = true;
                }
                task();
            }

        }
 
        public: 
            event_trigger_threadpool() {
                has_execution_completed = false;
                auto count = 1;
                for (int i = 0; i < count; i++) {
                    workers.push_back(std::thread(&event_trigger_threadpool::run_method, this));
                }
            }

            ~event_trigger_threadpool() {
                has_execution_completed = true;
                for (auto& worker: workers) {
                    worker.join();
                }
            }

            /**
            * Invoked by the Rules Engine on system initialization. 
            */ 
            static void set_commit_trigger(f_commit_trigger_t commit_trigger) {
                s_tx_commit_trigger = commit_trigger;
            }

            /**
             * Invoked by the Storage Engine during transaction commit to check whether the rules engine has
             * been initialized. We don't need synchronization here as the rules engine will be activated on startup, 
             * whereas transaction commit occurs much later.
             */
            static f_commit_trigger_t get_commit_trigger() {
                return s_tx_commit_trigger;
            }

            void add_trigger_task(gaia_xid_t transaction_id, std::vector<triggers::trigger_event_t> events) {
                // Pass vector of events by copy to capture for now.
                // We can avoid the copy with a custom allocator (if copying becomes a problem)
                // Todo - deep copy events post yi wen's changes?
                tasks.push([=] () {
                    s_tx_commit_trigger(transaction_id, std::move(events), true);
                });
            }        
    };
}
}
}
