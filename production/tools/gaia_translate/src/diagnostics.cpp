////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "diagnostics.h"

namespace gaia
{
namespace translation
{

std::unique_ptr<diagnostic_context_t> g_diag_ptr;

diagnostic_context_t& diag()
{
    return *g_diag_ptr;
}

} // namespace translation
} // namespace gaia
