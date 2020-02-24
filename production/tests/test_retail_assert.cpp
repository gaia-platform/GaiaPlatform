/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "constants.hpp"
#include "retail_assert.hpp"

using namespace std;
using namespace gaia::common;

void test_retail_assert();

int main()
{
    test_retail_assert();

    cout << endl << c_all_tests_passed << endl;
}

void test_retail_assert()
{
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** retail_assert tests started ***" << endl;
    cout << c_debug_output_separator_line_end << endl;

    try
    {
        retail_assert(true, "Unexpected triggering of retail assert!");
    }
    catch(const std::exception& e)
    {
        cout << "ERROR: An unexpected exception was thrown!" << endl;
        cerr << "ERROR: Exception message: " << e.what() << '\n';
    }

    cout << "PASSED: No exception was thrown by retail_assert on a true condition!" << endl;

    try
    {
        retail_assert(false, "Expected triggering of retail assert.");
    }
    catch(const std::exception& e)
    {
        cout << "PASSED: An exception was thrown by retail_assert on a false condition, as expected." << endl;
        cerr << "PASSED: Exception message: " << e.what() << '\n';
    }

    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** retail_assert tests ended ***" << endl;
    cout << c_debug_output_separator_line_end << endl;
}
