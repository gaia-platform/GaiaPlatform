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
    public:

    string m_message;

    gaia_exception(string message)
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

