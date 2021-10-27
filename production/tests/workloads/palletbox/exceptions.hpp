/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <exception>
#include <string>

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

    explicit palletbox_exception(std::string message)
        : m_message(std::move(message)){};

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
