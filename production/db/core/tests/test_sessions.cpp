////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia/exceptions.hpp"

#include "gaia_internal/db/db_test_base.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;

static constexpr int64_t c_session_sleep_millis = 10;

class session_test : public db_test_base_t
{
public:
    session_test()
        : db_test_base_t(false, false)
    {
    }
};

TEST_F(session_test, starting_multiple_sessions_on_same_thread_fail)
{
    gaia::db::begin_session();
    EXPECT_THROW(gaia::db::begin_session(), session_exists);
    EXPECT_THROW(gaia::db::begin_session(), session_exists);
    EXPECT_THROW(gaia::db::begin_ddl_session(), session_exists);
    gaia::db::end_session();

    gaia::db::begin_ddl_session();
    EXPECT_THROW(gaia::db::begin_ddl_session(), session_exists);
    EXPECT_THROW(gaia::db::begin_ddl_session(), session_exists);
    EXPECT_THROW(gaia::db::begin_session(), session_exists);
    gaia::db::end_session();
}

TEST_F(session_test, mix_ddl_and_regular_sessions_succeed)
{
    gaia::db::begin_ddl_session();
    gaia::db::end_session();

    gaia::db::begin_ddl_session();
    gaia::db::end_session();

    gaia::db::begin_session();
    gaia::db::end_session();

    gaia::db::begin_ddl_session();
    gaia::db::end_session();
}

TEST_F(session_test, concurrent_regular_sessions_succeed)
{
    std::vector<std::thread> session_threads;

    constexpr size_t c_num_concurrent_sessions = 10;

    for (size_t i = 0; i < c_num_concurrent_sessions; ++i)
    {
        session_threads.emplace_back([]() {
            begin_session();
            std::this_thread::sleep_for(std::chrono::milliseconds(c_session_sleep_millis));
            end_session();
        });
    }

    for (auto& thread : session_threads)
    {
        thread.join();
    }
}

TEST_F(session_test, only_one_ddl_session_running)
{
    constexpr int c_num_concurrent_sessions = 5;

    std::vector<std::thread> threads;
    std::atomic<uint32_t> num_active_sessions{0};

    for (int i = 0; i < c_num_concurrent_sessions; ++i)
    {
        // NOLINTNEXTLINE(performance-inefficient-vector-operation)
        threads.emplace_back([&num_active_sessions] {
            gaia::db::begin_ddl_session();
            num_active_sessions++;
            std::this_thread::sleep_for(std::chrono::milliseconds(c_session_sleep_millis));
            ASSERT_EQ(num_active_sessions, 1);
            num_active_sessions--;
            gaia::db::end_session();
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }
}

TEST_F(session_test, ddl_session_wait_regular_session)
{
    std::atomic<uint32_t> num_active_sessions{0};

    std::thread regular_session([&num_active_sessions] {
        gaia::db::begin_session();
        num_active_sessions++;
        std::this_thread::sleep_for(std::chrono::milliseconds(c_session_sleep_millis));
        num_active_sessions--;
        gaia::db::end_session();
    });

    std::thread ddl_session([&num_active_sessions] {
        gaia::db::begin_ddl_session();
        ASSERT_EQ(num_active_sessions, 0);
        gaia::db::end_session();
    });

    ddl_session.join();
    regular_session.join();
}

TEST_F(session_test, regular_session_wait_for_ddl_session)
{
    std::atomic<uint32_t> num_active_sessions{0};

    std::thread ddl_session([&num_active_sessions] {
        gaia::db::begin_ddl_session();
        num_active_sessions++;
        std::this_thread::sleep_for(std::chrono::milliseconds(c_session_sleep_millis));
        ASSERT_EQ(num_active_sessions, 1);
        num_active_sessions--;
        gaia::db::end_session();
    });

    std::thread regular_session([&num_active_sessions] {
        gaia::db::begin_session();
        ASSERT_EQ(num_active_sessions, 0);
        gaia::db::end_session();
    });

    ddl_session.join();
    regular_session.join();
}

TEST_F(session_test, ping_sessions_can_run_with_ddl_sessions)
{
    constexpr int c_num_concurrent_sessions = 5;

    std::vector<std::thread> threads;

    std::condition_variable running_ping_sessions_cv;
    std::mutex ping_session_mutex;

    std::atomic<int> num_ping_sessions = c_num_concurrent_sessions;

    threads.emplace_back([&] {
        gaia::db::begin_ddl_session();
        std::unique_lock lock(ping_session_mutex);

        running_ping_sessions_cv.wait(
            lock, [&] { return num_ping_sessions == 0; });

        gaia::db::end_session();
    });

    for (int i = 0; i < c_num_concurrent_sessions; ++i)
    {
        // NOLINTNEXTLINE(performance-inefficient-vector-operation)
        threads.emplace_back([&] {
            gaia::db::begin_ping_session();
            num_ping_sessions--;
            running_ping_sessions_cv.notify_one();
            gaia::db::end_session();
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }
}
