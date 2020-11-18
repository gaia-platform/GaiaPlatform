/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

namespace gaia
{
namespace direct_access
{

// This class has been introduced to enable us to differentiate between
// zero-length strings and null strings.  The C++ object api --gen-object-api
// represents strings as std::string by default and represents zero-length
// strings as null values.  To use this type instead of std::string, we pass
// the class name (gaia::direct_access::nullable_string_t) to the flatc compiler via
// the option --cpp-str-type.
struct nullable_string_t : std::string
{
    nullable_string_t()
        : is_null(true)
    {
    }

    nullable_string_t(const char* c_str, uint32_t)
        : std::string(c_str), is_null(c_str == nullptr)
    {
    }

    const char* c_str() const
    {
        if (is_null)
        {
            return nullptr;
        }
        return std::string::c_str();
    }

    nullable_string_t(const nullable_string_t&) = default;

    nullable_string_t& operator=(const nullable_string_t& nullable_string)
    {
        is_null = nullable_string.is_null;
        if (is_null)
        {
            clear();
        }
        else
        {
            std::string::operator=(nullable_string);
        }
        return *this;
    }

    nullable_string_t& operator=(const std::string& cpp_string)
    {
        is_null = false;
        std::string::operator=(cpp_string);
        return *this;
    }

    nullable_string_t& operator=(const char* c_str)
    {
        is_null = (c_str == nullptr);
        if (is_null)
        {
            clear();
        }
        else
        {
            std::string::operator=(c_str);
        }

        return *this;
    }

    // When a flatbuffer is created, it will call the empty() method.  Default behavior
    // is to set a nullptr if the nullable_string_t is zero-length.  We will only return true
    // if the nullable_string_t is null.
    bool empty() const
    {
        return is_null;
    }

private:
    bool is_null;
};

} // namespace direct_access
} // namespace gaia
