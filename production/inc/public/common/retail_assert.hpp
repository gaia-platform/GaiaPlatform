/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

#include "gaia_exception.hpp"

using namespace std;

namespace gaia {
/**
 * \addtogroup Gaia
 * @{
 */
namespace common {
/**
 * \addtogroup Common
 * @{
 */

/**
 * Thrown when a retail assert check has failed.
 */
class retail_assertion_failure : public gaia_exception {
   public:
    retail_assertion_failure(const string& message)
        : gaia_exception(message) {
    }
};

/**
 * Retail asserts are meant for important conditions that indicate internal errors.
 * They help us surface issues early, at the source.
 */
inline void retail_assert(bool condition, const string& message = "") {
    if (!condition) {
        throw retail_assertion_failure(message);
    }
}

/*@}*/
}  // namespace common
/*@}*/
}  // namespace gaia
