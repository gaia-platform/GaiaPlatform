/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "gaia_base.hpp"

namespace gaia 
{
namespace direct_access 
{

//
// Exception class implementations.
//
edc_invalid_object_type::edc_invalid_object_type(
    gaia_id_t id, 
    gaia_type_t expected, 
    const char* expected_type,
    gaia_type_t actual, 
    const char* type_name) 
{
    stringstream msg;
    msg << "Requesting Gaia type " << expected_type << "(" << expected << ") but object identified by "
        << id << " is type " << type_name << "(" << actual << ").";
    m_message = msg.str();
}

edc_invalid_member::edc_invalid_member(
    gaia_id_t id, 
    gaia_type_t parent, 
    const char* parent_type,
    gaia_type_t child, 
    const char* child_name) 
{
    stringstream msg;
    msg << "Attempting to remove record with Gaia type " << child_name << "(" << child << ") from parent "
        << id << " of type " << parent_type << "(" << parent << "), but record is not a member.";
    m_message = msg.str();
}

edc_inconsistent_list::edc_inconsistent_list(
    gaia_id_t id, 
    const char* parent_type, 
    gaia_id_t child, 
    const char* child_name) 
{
    stringstream msg;
    msg << "List is inconsistent, child points to parent " << id << " of type " << parent_type
        << ", but child (" << child << ", type " << child_name << ") is not in parent's list.";
    m_message = msg.str();
}

edc_unstored_row::edc_unstored_row(
    const char* parent_type, 
    const char* child_type) 
{
    stringstream msg;
    msg << "Cannot connect two objects until they have both been inserted (insert_row()), parent type is " <<
        parent_type << " and child type is " << child_type << ".";
    m_message = msg.str();
}

//
// Base gaia_base_t implementation.
//
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

} // direct_access
} // gaia
