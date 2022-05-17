/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia/exceptions.hpp"

#include "gaia_internal/db/db_test_base.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;

static constexpr int64_t c_regular_session_sleep_millis = 10;

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
            std::this_thread::sleep_for(std::chrono::milliseconds(c_regular_session_sleep_millis));
            end_session();
        });
    }

    for (auto& thread : session_threads)
    {
        thread.join();
    }
}

// Tests that we can't start Regular or DDL sessions while a DDL session is already running.
TEST_F(session_test, concurrent_ddl_sessions_fail)
{
    std::mutex main_thread_mutex;
    std::mutex ddl_thread_mutex;

    std::condition_variable ddl_session_started_cv;
    std::condition_variable main_assertions_completed_cv;

    std::thread ddl_session([&] {
        gaia::db::begin_ddl_session();
        // After the DDL session is started we can unlock the main thread.
        ddl_session_started_cv.notify_one();

        // Wait for the assertions in the main thread to be completed.
        unique_lock lock(ddl_thread_mutex);
        main_assertions_completed_cv.wait(lock);

        gaia::db::end_session();

        // The main thread is waiting for this DDL session to complete.
    });

    // Wait for the DDL session to be started.
    unique_lock lock(main_thread_mutex);
    ddl_session_started_cv.wait(lock);

    // It should not be possible to open any session (DDL or regular)
    // while a DDL session is running.
    ASSERT_THROW(gaia::db::begin_session(), session_failure);
    ASSERT_THROW(gaia::db::begin_ddl_session(), session_failure);

    // Let the DDL session thread complete the session.
    main_assertions_completed_cv.notify_one();

    // Wait for the DDL session to be completed.
    ddl_session.join();

    // Now that the DDL session is completed, ensure we can open new
    // normal ddl session.
    gaia::db::begin_ddl_session();
    gaia::db::end_session();

    // And a regular session.
    gaia::db::begin_session();
    gaia::db::end_session();
}

// Tests that we can't start a DDL session while a normal session is already running.
TEST_F(session_test, concurrent_ddl_regular_sessions_fail)
{
    std::mutex main_thread_mutex;
    std::mutex regular_thread_mutex;

    std::condition_variable regular_session_started_cv;
    std::condition_variable main_assertions_completed_cv;

    std::thread ddl_session([&] {
        gaia::db::begin_session();
        // After the regular session is started we can unlock the main thread.
        regular_session_started_cv.notify_one();

        // Wait for the assertions in the main thread to be completed.
        unique_lock lock(regular_thread_mutex);
        main_assertions_completed_cv.wait(lock);

        gaia::db::end_session();

        // The main thread is waiting for this Regular session to complete.
    });

    // Wait for the DDL session to be started.
    unique_lock lock(main_thread_mutex);
    regular_session_started_cv.wait(lock);

    // It should not be possible to open a DDL session while a
    // regular session is running.
    ASSERT_THROW(gaia::db::begin_ddl_session(), session_failure);

    // Let the DDL session thread complete the session.
    main_assertions_completed_cv.notify_one();

    // Wait for the DDL session to be completed.
    ddl_session.join();

    // Now that the DDL session is completed, ensure we can open new
    // normal ddl session.
    gaia::db::begin_ddl_session();
    gaia::db::end_session();

    // And a regular session.
    gaia::db::begin_session();
    gaia::db::end_session();
}
