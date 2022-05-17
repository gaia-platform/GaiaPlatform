/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <csignal>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <gtest/gtest.h>

#include "gaia/db/db.hpp"

#include "gaia_internal/common/scope_guard.hpp"
#include "gaia_internal/common/timer.hpp"
#include "gaia_internal/db/db_client_config.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"

#include "db_helpers.hpp"

using namespace gaia::db;
using namespace gaia::common;
using scope_guard::make_scope_guard;

class latch
{
public:
    latch() = default;
    ~latch() = default;

    void reset(size_t count)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_count = count;
    }

    void wait()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_count > 0)
        {
            m_cv.wait(lock, [this]() { return m_count == 0; });
        }
    }

    void count_down()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_count > 0)
        {
            m_count--;
            m_cv.notify_all();
        }
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    size_t m_count{0};
};

/**
 * Google test fixture object.  This class is used by each
 * test case below.  SetUp() is called before each test is run
 * and TearDown() is called after each test case is done.
 */
class db__core__concurrent_db_client__test : public ::testing::Test
{
public:
    static constexpr char c_even_value[] = "ping";
    static constexpr char c_odd_value[] = "pong";

    static constexpr gaia_type_t c_object_type{1};

    // Initialized to false state.
    static inline std::atomic<bool> s_should_exit{};
    static_assert(std::atomic<bool>::is_always_lock_free);

    std::atomic<size_t> m_total_txn_count{};

    // An array of DB object IDs, where each object is only updated by a single thread.
    // This stores the underlying type because int_type_t isn't copyable so doesn't work with forwarding.
    std::vector<gaia_id_t::value_type> m_object_ids{};

    // A barrier that each worker arrives at after setup and before doing work.
    // The controller waits on this barrier before arriving at the controller_ready barrier.
    latch m_workers_ready{};

    // A barrier that each worker waits on after setup and before doing work.
    // The controller arrives at this barrier after waiting on the workers_ready barrier.
    latch m_controller_ready{};

    static void handle_termination(int signal)
    {
        std::cerr << "Caught signal " << strsignal(signal) << std::endl;
        std::cerr << "Termination requested, exiting early..." << std::endl;
        std::exit(1);
    }

    static void register_signal_handler()
    {
        struct sigaction sigact
        {
        };
        sigact.sa_handler = &handle_termination;
        sigemptyset(&sigact.sa_mask);
        sigact.sa_flags = 0;
        if (-1 == sigaction(SIGINT, &sigact, NULL))
        {
            throw_system_error("sigaction(SIGINT) failed!");
        }
        if (-1 == sigaction(SIGTERM, &sigact, NULL))
        {
            throw_system_error("sigaction(SIGTERM) failed!");
        }
    }

    void init_data(size_t num_workers)
    {
        begin_session();
        {
            m_object_ids.clear();

            // Create a new ID for each DB object we create.
            for (size_t worker_id = 0; worker_id < num_workers; ++worker_id)
            {
                m_object_ids.push_back(allocate_id().value());
            }
        }
        end_session();
    }

    void worker(gaia_id_t::value_type object_id)
    {
        begin_session();
        const auto session_cleanup = scope_guard::make_scope_guard([]() { end_session(); });

        // Initialize this worker's local object reference.
        begin_transaction();
        auto obj = gaia_ptr_t::create(
            gaia_id_t(object_id), c_object_type, 0, sizeof(c_even_value), c_even_value);
        commit_transaction();

        // Signal the controller that we're ready.
        m_workers_ready.count_down();

        // Wait for the controller to signal that it's ready.
        m_controller_ready.wait();

        size_t local_txn_count = 0;
        while (!s_should_exit.load(std::memory_order_relaxed))
        {
            // Pick the alternating value to update.
            size_t new_size = (local_txn_count & 1) ? sizeof(c_odd_value) : sizeof(c_even_value);
            const char* new_value = (local_txn_count & 1) ? c_odd_value : c_even_value;

            // Commit the update.
            begin_transaction();
            {
                obj.update_payload(new_size, new_value);
            }
            commit_transaction();

            ++local_txn_count;
        }

        // Update the global counter.
        m_total_txn_count += local_txn_count;
    }
};

TEST_F(db__core__concurrent_db_client__test, DISABLED_concurrent_update_throughput)
{
    // Handle termination gracefully.
    db__core__concurrent_db_client__test::register_signal_handler();

    for (size_t num_workers = 1; num_workers <= std::thread::hardware_concurrency(); ++num_workers)
    {
        m_total_txn_count = 0;

        init_data(num_workers);

        m_workers_ready.reset(num_workers);
        m_controller_ready.reset(1);

        // Reset the shutdown flag.
        s_should_exit.store(false, std::memory_order_relaxed);

        // Spawn each worker before waiting on the workers_ready barrier.
        std::vector<std::thread> worker_threads{};
        for (size_t worker_id = 0; worker_id < num_workers; ++worker_id)
        {
            worker_threads.emplace_back(std::thread([=] { worker(m_object_ids[worker_id]); }));
        }

        // Wait for all workers to be ready.
        m_workers_ready.wait();

        // Start the clock.
        auto start_time = gaia::common::timer_t::get_time_point();

        // Signal all workers to begin.
        m_controller_ready.count_down();

        // Sleep for 1 minute.
        std::this_thread::sleep_for(std::chrono::seconds(60));

        // Signal the shutdown flag.
        s_should_exit.store(true, std::memory_order_relaxed);

        // Wait for all workers to finish.
        for (auto& worker_thread : worker_threads)
        {
            worker_thread.join();
        }

        // Compute elapsed time.
        auto elapsed_time_secs = gaia::common::timer_t::ns_to_s(gaia::common::timer_t::get_duration(start_time));

        // Compute transactions/second.
        auto tps = static_cast<size_t>(std::ceil(static_cast<double>(m_total_txn_count) / elapsed_time_secs));

        std::cout << num_workers << " " << tps << std::endl;
    }
}
