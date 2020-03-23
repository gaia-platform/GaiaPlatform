/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <exception>
#include <string>

using namespace std;

namespace gaia
{
namespace common
{

class gaia_exception : public exception
{
    protected:

    string m_message;

    public:
    gaia_exception() = default;

    gaia_exception(const string& message)
    {
        m_message = message;
    }

    virtual const char* what() const throw()
    {
        return m_message.c_str();
    }
};

}
}

