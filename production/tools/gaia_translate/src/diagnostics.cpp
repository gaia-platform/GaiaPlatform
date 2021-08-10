/////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////
#include "diagnostics.h"

namespace gaia
{
namespace translation
{

diagnostic_t::diagnostic_t(const char* message)
    : m_message(message)
{
    std::cerr << "error: ";
}

diagnostic_t::~diagnostic_t()
{
    /*
    if (*m_message != '\0')
    {
        std::cerr << m_message;
    }
    */
    std::cerr << m_message << std::endl;
}

diagnostic_t& diagnostic_t::operator<<(const std::string& arg)
{
    output_next_arg(arg);
    return *this;
}

diagnostic_t& diagnostic_t::operator<<(int arg)
{
    output_next_arg(arg);
    return *this;
}

template <typename T_arg>
void diagnostic_t::output_next_arg(T_arg arg)
{
    for (; *m_message != '\0'; m_message++)
    {
        if (*m_message == '%')
        {
            std::cerr << arg;
            m_message++;
            return;
        }
        std::cerr << *m_message;
    }
}

// Specializations we support.
template void diagnostic_t::output_next_arg<const std::string&>(const std::string&);
template void diagnostic_t::output_next_arg<int>(int);

diagnostic_t diag_err(const char* message)
{
    return diagnostic_t(message);
}

} // namespace translation
} // namespace gaia
