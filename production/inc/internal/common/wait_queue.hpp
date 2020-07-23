/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <thread>
#include <queue>
#include <condition_variable>

namespace gaia {
namespace db {
    
// This class implements a simple thread safe queue (using dumb locks)
template<typename T>
class wait_queue_t {
        
    private:
        std::mutex mutex;
        std::condition_variable queue_has_data;
        std::queue<T> queue;

    public:
        wait_queue_t() = default;

        void push(T&& val) {
            std::lock_guard<std::mutex> lock (mutex);
            queue.push(std::move(val));
            queue_has_data.notify_one();
        }

        void pop(T& val) {
            std::unique_lock<std::mutex> lock (mutex);
            queue_has_data.wait(lock, [this]() {return !queue.empty(); });
            val = std::move(queue.front());
            queue.pop();
        }        
    };

}
}
