/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "constants.hpp"
#include "retail_assert.hpp"

#include "access_control.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::db::memory_manager;

void test_access_control();

int main()
{
    test_access_control();

    cout << endl << c_all_tests_passed << endl;
}

void test_access_control()
{
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << ">>> access_control_t tests started <<<" << endl;
    cout << c_debug_output_separator_line_end << endl;

    cout << "sizeof(access_control_t) = " << sizeof(access_control_t) << endl;

    access_control_t access_control;
    access_control_t second_access_control;
    access_lock_type_t existing_access;

    {
        auto_access_control_t auto_access;

        retail_assert(
            auto_access.try_to_lock_access(&access_control, access_lock_type_t::remove),
            "ERROR: Auto accessor failed to acquire available access!");
        retail_assert(
            access_control.access_lock == access_lock_type_t::remove,
            "ERROR: Access control does not indicate expected remove value!");
        retail_assert(
            access_control.readers_count == 1,
            "ERROR: Access control does not indicate expected reader count value of 1!");
        cout << "PASSED: First call of try_to_lock_access() has succeeded as expected!" << endl;

        retail_assert(
            auto_access.try_to_lock_access(&access_control, access_lock_type_t::update),
            "ERROR: Auto accessor failed to release and reacquire available access!");
        retail_assert(
            access_control.access_lock == access_lock_type_t::update,
            "ERROR: Access control does not indicate expected update value!");
        retail_assert(
            access_control.readers_count == 1,
            "ERROR: Access control does not indicate expected reader count value of 1!");
        cout << "PASSED: Second call of try_to_lock_access() has succeeded as expected!" << endl;

        auto_access_control_t second_auto_access;

        retail_assert(
            !second_auto_access.try_to_lock_access(&access_control, access_lock_type_t::remove, existing_access),
            "ERROR: Auto accessor managed to acquire already granted access!");
        retail_assert(
            existing_access == access_lock_type_t::update,
            "ERROR: Unexpected existing access was returned!");
        cout << "PASSED: Cannot re-lock existing locked access!" << endl;

        retail_assert(
            second_auto_access.try_to_lock_access(&second_access_control, access_lock_type_t::remove, existing_access),
            "ERROR: Auto accessor failed to acquire available access!");
        retail_assert(
            second_access_control.access_lock == access_lock_type_t::remove,
            "ERROR: Access control does not indicate expected remove value!");
        retail_assert(
            second_access_control.readers_count == 1,
            "ERROR: Access control does not indicate expected reader count value of 1!");
        retail_assert(
            existing_access == access_lock_type_t::none,
            "ERROR: Unexpected existing access was returned!");
        cout << "PASSED: Can lock different unlocked access!" << endl;

        auto_access_control_t third_auto_access;

        third_auto_access.mark_access(&access_control);
        retail_assert(
            access_control.readers_count == 2,
            "ERROR: Access control does not indicate expected reader count value of 2!");
        cout << "PASSED: Can mark access on locked control!" << endl;

        third_auto_access.mark_access(&second_access_control);
        retail_assert(
            access_control.readers_count == 1,
            "ERROR: Access control does not indicate expected reader count value of 1!");
        retail_assert(
            second_access_control.readers_count == 2,
            "ERROR: Access control does not indicate expected reader count value of 2!");
        cout << "PASSED: Can release and mark access on different locked control!" << endl;
    }

    retail_assert(
        access_control.access_lock == access_lock_type_t::none,
        "ERROR: Access control has not reverted to expected none value!");
    retail_assert(
        access_control.readers_count == 0,
        "ERROR: Access control does not indicate expected reader count value of 0!");
    retail_assert(
        second_access_control.access_lock == access_lock_type_t::none,
        "ERROR: Access control has not reverted to expected none value!");
    retail_assert(
        access_control.readers_count == 0,
        "ERROR: Access control does not indicate expected reader count value of 0!");

    {
        auto_access_control_t auto_access;

        auto_access.mark_access(&access_control);
        retail_assert(
            access_control.access_lock == access_lock_type_t::none,
            "ERROR: Access control does not indicate expected none value!");
        retail_assert(
            access_control.readers_count == 1,
            "ERROR: Access control does not indicate expected reader count value of 1!");
        cout << "PASSED: mark_access() has succeeded as expected!" << endl;

        retail_assert(
            auto_access.try_to_lock_access(access_lock_type_t::remove),
            "ERROR: Auto accessor failed to acquire available access!");
        retail_assert(
            access_control.access_lock == access_lock_type_t::remove,
            "ERROR: Access control does not indicate expected remove value!");
        retail_assert(
            access_control.readers_count == 1,
            "ERROR: Access control does not indicate expected reader count value of 1!");
        cout << "PASSED: try_to_lock_access() has succeeded as expected!" << endl;

        auto_access.release_access();
        retail_assert(
            access_control.access_lock == access_lock_type_t::none,
            "ERROR: Access control does not indicate expected none value!");
        retail_assert(
            access_control.readers_count == 0,
            "ERROR: Access control does not indicate expected reader count value of 0!");
        cout << "PASSED: release_access() has succeeded as expected!" << endl;

        // Re-acquire access lock so we can test releasing lock only.
        retail_assert(
            auto_access.try_to_lock_access(&access_control, access_lock_type_t::remove),
            "ERROR: Auto accessor failed to acquire available access!");
        retail_assert(
            access_control.access_lock == access_lock_type_t::remove,
            "ERROR: Access control does not indicate expected remove value!");
        retail_assert(
            access_control.readers_count == 1,
            "ERROR: Access control does not indicate expected reader count value of 1!");
        cout << "PASSED: try_to_lock_access() has succeeded as expected!" << endl;

        auto_access.release_access_lock();
        retail_assert(
            access_control.access_lock == access_lock_type_t::none,
            "ERROR: Access control does not indicate expected none value!");
        retail_assert(
            access_control.readers_count == 1,
            "ERROR: Access control does not indicate expected reader count value of 1!");
        cout << "PASSED: release_access_lock() has succeeded as expected!" << endl;
    }

    retail_assert(
        access_control.access_lock == access_lock_type_t::none,
        "ERROR: Access control has not reverted to expected none value!");
    retail_assert(
        access_control.readers_count == 0,
        "ERROR: Access control does not indicate expected reader count value of 0!");

    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** access_control_t tests ended ***" << endl;
    cout << c_debug_output_separator_line_end << endl;
}
