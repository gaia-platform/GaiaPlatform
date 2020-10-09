/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string.h>

#include <string>
#include <sstream>

#include "gaia_exception.hpp"

namespace gaia {
namespace common {

/**
 * Thrown when a system call returns an error.
 */
class system_error : public gaia_exception {
   public:
    system_error(const string& message, int err = 0)
        : gaia_exception(message) {
        m_err = err;
    }
    int get_errno() {
        return m_err;
    }
   private:
    int m_err;
};

inline void throw_system_error(const string& user_info, const int err = errno) {
    std::stringstream ss;
    ss << user_info << " - " << (::strerror(err));
    throw system_error(ss.str(), err);
}

}  // namespace common
}  // namespace gaia
