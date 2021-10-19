/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <exception>

namespace gaia
{
namespace palletbox
{
namespace exceptions
{

class palletbox_exception : public std::exception
{
public:
    explicit palletbox_exception(const char* message)
        : m_msg(message){};

    explicit palletbox_exception(const std::string& message)
        : m_msg(message){};

    ~palletbox_exception() noexcept override = default;

    [[nodiscard]] const char* what() const noexcept override
    {
        return m_msg.c_str();
    }

protected:
    std::string m_msg;
};

} // namespace exceptions
} // namespace palletbox
} // namespace gaia
