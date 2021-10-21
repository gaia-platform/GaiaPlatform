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
        : m_message(message){};

    explicit palletbox_exception(const std::string& message)
        : m_message(message){};

    ~palletbox_exception() noexcept override = default;

    [[nodiscard]] const char* what() const noexcept override
    {
        return m_message.c_str();
    }

protected:
    std::string m_message;
};

} // namespace exceptions
} // namespace palletbox
} // namespace gaia
