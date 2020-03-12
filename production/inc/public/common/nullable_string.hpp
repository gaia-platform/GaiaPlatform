/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

namespace gaia {
namespace common {

struct nullable_string_t : std::string
{
    nullable_string_t()
    : is_null(true)
    {}

    nullable_string_t(const char * c_str, uint32_t size)
    : std::string(c_str), is_null(c_str == nullptr)
    { (void)size;}

    const char * c_str() const
    {
        if (is_null) {
            return nullptr;
        }
        return std::string::c_str();
    }

    nullable_string_t& operator=(const nullable_string_t& nstr)
    {
        is_null = nstr.is_null;
        if (!nstr.is_null) {
            std::string::operator=(nstr);
        }
        return *this;
    }

    nullable_string_t& operator=(const std::string& str)
    {
        is_null = false;
        std::string::operator=(str);
        return *this;
    }

    nullable_string_t& operator=(const char * c_str)
    {
        is_null = (c_str == nullptr);
        if (c_str) {
            std::string::operator=(c_str);
        }

        return *this;
    }

    // When a flatbuffer is created, it will call the empty() method.  Default behavior
    // is to set a nullptr if the nullable_string_t is zero-length.  We will only return true
    // if the nullable_string_t is null
    bool empty() const
    {
        return is_null;
    }

private:
    bool is_null;
};

} // gaia
} // common
