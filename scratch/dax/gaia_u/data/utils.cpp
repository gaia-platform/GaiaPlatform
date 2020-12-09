/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "utils.hpp"
#include <cstring>

using namespace std;

// Move these out of loader since they will be used to load and dump data
int64_t utils_t::convert_date(const char* date)
{
    tm the_date;
    memset(&the_date, 0, sizeof(tm));
    strptime(date, c_date_format, &the_date);
    return mktime(&the_date);
}

string utils_t::convert_date(int64_t datestamp)
{
    char buffer[12];
    tm* t_ptr = localtime(&datestamp);
    strftime(buffer, sizeof(buffer), c_date_format, t_ptr);
    return string(buffer);
}

int64_t utils_t::convert_time(const char* time)
{
    tm the_time;
    memset(&the_time, 0, sizeof(tm));
    strptime(time, c_time_format, &the_time);
    return mktime(&the_time);
}

string utils_t::convert_time(int64_t timestamp)
{
    char buffer[12];
    tm* t_ptr = localtime(&timestamp);
    strftime(buffer, sizeof(buffer),c_time_format, t_ptr);
    return string(buffer);
}
