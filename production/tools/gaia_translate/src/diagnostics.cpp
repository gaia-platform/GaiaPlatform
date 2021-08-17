/////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////
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
