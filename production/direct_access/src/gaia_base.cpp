/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "gaia_base.hpp"

// NOTE: This is included included by gaia_object.hpp as this is a template
// implementation file.  Because the template specializations of gaia_object_t are
// created by user-defined schema, we don't know what they will be apriori.  I have
// pulled them out of gaia_object.hpp, however, to include readability of the
// facilities gaia_object_t decleartions.

namespace gaia 
{
namespace direct 
{

/**
 * Exception class implementations
 */
edc_invalid_object_type::edc_invalid_object_type(
    gaia_id_t id, 
    gaia_type_t expected, 
    const char* expected_type,
    gaia_type_t actual, 
    const char* type_name) 
{
    stringstream msg;
    msg << "requesting Gaia type " << expected_type << "(" << expected << ") but object identified by "
        << id << " is type " << type_name << "(" << actual << ")";
    m_message = msg.str();
}


/**
 * gaia_base_t implementation
 */
gaia_base_t::gaia_base_t(const char* gaia_typename)
: gaia_base_t(0, gaia_typename)
{
}

gaia_base_t::gaia_base_t(gaia_id_t id, const char * gaia_typename)
: m_id(id)
, m_typename(gaia_typename)
{
    set_tx_hooks();
}

void gaia_base_t::set_tx_hooks()
{
    if (!s_tx_hooks_installed)
    {
        // Do not overwrite an already established hook.  This could happen
        // if an application has subscribed to transaction events.
        if (!gaia::db::s_tx_commit_hook)
        {
            gaia::db::s_tx_commit_hook = commit_hook;
        }
        s_tx_hooks_installed = true;
    }
}

void gaia_base_t::commit_hook()
{
    for (auto it = s_gaia_tx_cache.begin(); it != s_gaia_tx_cache.end(); ++it)
    {
        it->second->reset(true);
    }
    s_gaia_tx_cache.clear();
}

} // direct
} // gaia
