/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

namespace gaia {

struct string : std::string
{
    string() 
    : is_null(true)
    {}

    string(const char * c_str, uint32_t size)
    : std::string(c_str), is_null(c_str == nullptr)
    { (void)size;}

    const char * c_str() const
    {
        if (is_null) {
            return nullptr;
        }
        return std::string::c_str();
    }

    string& operator=(const string& nstr)
    {
        is_null = nstr.is_null;
        if (!nstr.is_null) {
            std::string::operator=(nstr);
        }
        return *this;
    }

    string& operator=(const std::string& str)
    {
        is_null = false;
        std::string::operator=(str);
        return *this;
    }

    string& operator=(const char * c_str)
    {
        is_null = (c_str == nullptr);
        if (c_str) {
            std::string::operator=(c_str);
        }

        return *this;
    }

    // when a flatbuffer is created, it will call the empty() method.  Default behavior
    // is to set a nullptr if the string is zero-length.  We will only return true
    // if the string is null
    bool empty() const
    {
        return is_null;
    }

private:
    bool is_null;
};

} // gaia