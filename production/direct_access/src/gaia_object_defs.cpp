/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_base.hpp"
using namespace gaia::direct;

gaia_base_t::id_cache_t gaia_base_t::s_gaia_cache;
gaia_base_t::id_cache_t gaia_base_t::s_gaia_tx_cache;
bool gaia_base_t::s_tx_hooks_installed = false;
