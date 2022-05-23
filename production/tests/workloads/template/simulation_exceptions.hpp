////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <exception>
#include <string>

namespace gaia
{
namespace simulation
{
namespace exceptions
{

class simulation_exception : public std::exception
{
public:
    explicit simulation_exception(const char* message)
        : m_message(message){};

    explicit simulation_exception(std::string message)
        : m_message(std::move(message)){};

    ~simulation_exception() noexcept override = default;

    [[nodiscard]] const char* what() const noexcept override
    {
        return m_message.c_str();
    }

protected:
    std::string m_message;
};

} // namespace exceptions
} // namespace simulation
} // namespace gaia
