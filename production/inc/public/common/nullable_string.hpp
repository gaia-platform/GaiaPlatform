/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

namespace gaia {
namespace common {

// The C++ object api --gen-object-api represents strings as std::string by default.
// On deserialization, null pointers in the database get converted to zero-lengh
// strings. On serialization, zero-length strings get converted back to null pointers.
// In order to be able to differentiate between zero-length strings and null strings,
// this class is used. This class is used by passing the class name
// (gaia::common::nullable_string_t) to the flatc compiler option --cpp-str-type
// to use this type instead of std::string.
struct nullable_string_t : std::string
{
    nullable_string_t()
        : is_null(true)
    {
    }

    nullable_string_t(const char *c_str, uint32_t size)
        : std::string(c_str), is_null(c_str == nullptr)
    {
        (void)size;
    }

    const char *c_str() const
    {
        if (is_null) {
            return nullptr;
        }
        return std::string::c_str();
    }

    nullable_string_t &operator=(const nullable_string_t &nullable_string)
    {
        is_null = nullable_string.is_null;
        if (!nullable_string.is_null) {
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
