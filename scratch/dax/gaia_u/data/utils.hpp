/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <time.h>
#include <string>

// provide an option for shared memory file name
// have the client

// Simplify to just use getline and string find...
// getline does have a delimter but we want both \n and ,
// Check in sources somewhere!

// Weekend goals:

// Parse file input
// display in form with tree controls
// Python binding layer
// Create a registration form!

class utils_t
{
public:
    // Date Format is %m/%d/%Y (ex: 12/20/2020)
    static inline const char* c_date_format = "%m/%d/%Y";
    // Format is %I:%M %p (ex: 01:20 PM)
    static inline const char* c_time_format = "%I:%M %p";

    // Move these out of loader since they will be used to load and dump data
    static int64_t convert_date(const char* date);
    static std::string convert_date(int64_t datestamp);
    static int64_t convert_time(const char* time);
    static std::string convert_time(int64_t timestamp);
};
