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
    cout << ">>> EAccessControl tests started <<<" << endl;
    cout << c_debug_output_separator_line_end << endl;

    cout << "sizeof(AccessControl) = " << sizeof(AccessControl) << endl;

    AccessControl accessControl;
    AccessControl secondAccessControl;
    EAccessLockType existingAccess;

    {
        CAutoAccessControl autoAccess;

        retail_assert(
            autoAccess.try_to_lock_access(&accessControl, alt_Remove),
            "ERROR: Auto accessor failed to acquire available access!");
        retail_assert(
            accessControl.accessLock == alt_Remove,
            "ERROR: Access control does not indicate expected alt_Remove value!");
        retail_assert(
            accessControl.readersCount == 1,
            "ERROR: Access control does not indicate expected reader count value of 1!");
        cout << "PASSED: First call of TryToLockAccess() has succeeded as expected!" << endl;

        retail_assert(
            autoAccess.try_to_lock_access(&accessControl, alt_Update),
            "ERROR: Auto accessor failed to release and reacquire available access!");
        retail_assert(
            accessControl.accessLock == alt_Update,
            "ERROR: Access control does not indicate expected alt_Update value!");
        retail_assert(
            accessControl.readersCount == 1,
            "ERROR: Access control does not indicate expected reader count value of 1!");
        cout << "PASSED: Second call of TryToLockAccess() has succeeded as expected!" << endl;

        CAutoAccessControl secondAutoAccess;

        retail_assert(
            !secondAutoAccess.try_to_lock_access(&accessControl, alt_Remove, existingAccess),
            "ERROR: Auto accessor managed to acquire already granted access!");
        retail_assert(
            existingAccess == alt_Update,
            "ERROR: Unexpected existing access was returned!");
        cout << "PASSED: Cannot re-lock existing locked access!" << endl;

        retail_assert(
            secondAutoAccess.try_to_lock_access(&secondAccessControl, alt_Remove, existingAccess),
            "ERROR: Auto accessor failed to acquire available access!");
        retail_assert(
            secondAccessControl.accessLock == alt_Remove,
            "ERROR: Access control does not indicate expected alt_Remove value!");
        retail_assert(
            secondAccessControl.readersCount == 1,
            "ERROR: Access control does not indicate expected reader count value of 1!");
        retail_assert(
            existingAccess == alt_None,
            "ERROR: Unexpected existing access was returned!");
        cout << "PASSED: Can lock different unlocked access!" << endl;

        CAutoAccessControl thirdAutoAccess;

        thirdAutoAccess.mark_access(&accessControl);
        retail_assert(
            accessControl.readersCount == 2,
            "ERROR: Access control does not indicate expected reader count value of 2!");
        cout << "PASSED: Can mark access on locked control!" << endl;

        thirdAutoAccess.mark_access(&secondAccessControl);
        retail_assert(
            accessControl.readersCount == 1,
            "ERROR: Access control does not indicate expected reader count value of 1!");
        retail_assert(
            secondAccessControl.readersCount == 2,
            "ERROR: Access control does not indicate expected reader count value of 2!");
        cout << "PASSED: Can release and mark access on different locked control!" << endl;
    }

    retail_assert(
        accessControl.accessLock == alt_None,
        "ERROR: Access control has not reverted to expected alt_None value!");
    retail_assert(
        accessControl.readersCount == 0,
        "ERROR: Access control does not indicate expected reader count value of 0!");
    retail_assert(
        secondAccessControl.accessLock == alt_None,
        "ERROR: Access control has not reverted to expected alt_None value!");
    retail_assert(
        accessControl.readersCount == 0,
        "ERROR: Access control does not indicate expected reader count value of 0!");

    {
        CAutoAccessControl autoAccess;

        autoAccess.mark_access(&accessControl);
        retail_assert(
            accessControl.accessLock == alt_None,
            "ERROR: Access control does not indicate expected alt_None value!");
        retail_assert(
            accessControl.readersCount == 1,
            "ERROR: Access control does not indicate expected reader count value of 1!");
        cout << "PASSED: MarkAccess() has succeeded as expected!" << endl;

        retail_assert(
            autoAccess.try_to_lock_access(alt_Remove),
            "ERROR: Auto accessor failed to acquire available access!");
        retail_assert(
            accessControl.accessLock == alt_Remove,
            "ERROR: Access control does not indicate expected alt_Remove value!");
        retail_assert(
            accessControl.readersCount == 1,
            "ERROR: Access control does not indicate expected reader count value of 1!");
        cout << "PASSED: TryToLockAccess() has succeeded as expected!" << endl;

        autoAccess.release_access();
        retail_assert(
            accessControl.accessLock == alt_None,
            "ERROR: Access control does not indicate expected alt_None value!");
        retail_assert(
            accessControl.readersCount == 0,
            "ERROR: Access control does not indicate expected reader count value of 0!");
        cout << "PASSED: ReleaseAccess() has succeeded as expected!" << endl;

        // Re-acquire access lock so we can test releasing lock only.
        retail_assert(
            autoAccess.try_to_lock_access(&accessControl, alt_Remove),
            "ERROR: Auto accessor failed to acquire available access!");
        retail_assert(
            accessControl.accessLock == alt_Remove,
            "ERROR: Access control does not indicate expected alt_Remove value!");
        retail_assert(
            accessControl.readersCount == 1,
            "ERROR: Access control does not indicate expected reader count value of 1!");
        cout << "PASSED: TryToLockAccess() has succeeded as expected!" << endl;

        autoAccess.release_access_lock();
        retail_assert(
            accessControl.accessLock == alt_None,
            "ERROR: Access control does not indicate expected alt_None value!");
        retail_assert(
            accessControl.readersCount == 1,
            "ERROR: Access control does not indicate expected reader count value of 1!");
        cout << "PASSED: ReleaseAccessLock() has succeeded as expected!" << endl;
    }

    retail_assert(
        accessControl.accessLock == alt_None,
        "ERROR: Access control has not reverted to expected alt_None value!");
    retail_assert(
        accessControl.readersCount == 0,
        "ERROR: Access control does not indicate expected reader count value of 0!");

    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** EAccessControl tests ended ***" << endl;
    cout << c_debug_output_separator_line_end << endl;
}
