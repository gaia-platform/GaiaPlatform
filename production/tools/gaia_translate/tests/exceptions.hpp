/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <exception>

namespace gaia
{
namespace amr_swarm
{
namespace exceptions
{

class amr_exception : public std::exception
{
public:
    explicit amr_exception(const char* message)
        : m_msg(message){};

    explicit amr_exception(const std::string& message)
        : m_msg(message){};

    ~amr_exception() noexcept override = default;

    [[nodiscard]] const char* what() const noexcept override
    {
        return m_msg.c_str();
    }

protected:
    std::string m_msg;
};

} // namespace exceptions
} // namespace amr_swarm
} // namespace gaia
