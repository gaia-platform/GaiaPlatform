/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "gaia_base.hpp"

#include "gaia_db.hpp"

namespace gaia
{
namespace direct_access
{

//
// Exception class implementations.
//
edc_invalid_object_type::edc_invalid_object_type(gaia_id_t id, gaia_type_t expected_type, const char* expected_typename,
                                                 gaia_type_t actual_type)
{
    stringstream msg;
    msg << "Requesting Gaia type " << expected_typename << "(" << expected_type << ") but object identified by " << id
        << " is type (" << actual_type << ").";
    m_message = msg.str();
}

edc_invalid_member::edc_invalid_member(gaia_id_t id, gaia_type_t parent, const char* parent_type, gaia_type_t child,
                                       const char* child_name)
{
    stringstream msg;
    msg << "Attempting to remove record with Gaia type " << child_name << "(" << child << ") from parent " << id
        << " of type " << parent_type << "(" << parent << "), but record is not a member.";
    m_message = msg.str();
}

edc_inconsistent_list::edc_inconsistent_list(gaia_id_t id, const char* parent_type, gaia_id_t child,
                                             const char* child_name)
{
    stringstream msg;
    msg << "List is inconsistent, child points to parent " << id << " of type " << parent_type << ", but child ("
        << child << ", type " << child_name << ") is not in parent's list.";
    m_message = msg.str();
}

edc_invalid_state::edc_invalid_state(const char* parent_type, const char* child_type)
{
    stringstream msg;
    msg << "Object(s) have invalid state, no ID exists. Parent type is " << parent_type << " and child type is "
        << child_type << ".";
    m_message = msg.str();
}

edc_already_inserted::edc_already_inserted(gaia_id_t parent, const char* parent_type)
{
    stringstream msg;
    msg << "The object being inserted is a member of this same list type but a different owner. "
           "The owner object type is "
        << parent_type << ", and id is " << parent << ".";
    m_message = msg.str();
}

} // namespace direct_access
} // namespace gaia
