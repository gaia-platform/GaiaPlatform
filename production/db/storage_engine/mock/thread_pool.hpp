/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
// This file implements a thread safe lock based queue.

#pragma once
#include "thread_safe_queue.hpp"
#include <functional>
#include <thread>
#include <vector>
#include <cassert>
#include <condition_variable>
#include <atomic>
#include <memory>

namespace gaia {
namespace db {

    class threadPool {
        private:
            std::atomic_bool done;
            //todo(msj) update to handle waitable tasks too
            lockBasedQueue<std::function<void()>> tasks;
            std::vector<std::thread> workers;

            void run_method() {
                if (!done) {
                    std::function<void()> task;
                    tasks.pop(task);
                    task();
                }
            }

        public: 
            threadPool() {
                done = false;
                auto count = 1;
                assert(count != 0);
                for (int i = 0; i < count; i++) {
                    workers.push_back(std::thread(&threadPool::run_method, this));
                }
            }

            ~threadPool() {
                done = true;
                for (auto& worker: workers) {
                    worker.join();
                }
            }

            template<typename F, typename... Args>
            void add_task(F&& f, Args&&... args) {
                tasks.push([&](){ f(args...); });
            }

            void add_trigger_task(int64_t trid, size_t event_count, shared_ptr<std::vector<unique_ptr<triggers::trigger_event_t>>> e) {
                tasks.push([=] () {
                    triggers::event_trigger::commit_trigger_(trid, e, event_count, true);
                });
            }
    };
}
}
