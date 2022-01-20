/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <memory>

#include "gaia/common.hpp"

namespace gaia
{
namespace db
{

class gaia_ptr_t;

/*
 * This namespace provides 'gaia_ptr_t' operations that include higher level
 * functionality like referential integrity checks and value-linked relationship
 * auto-connection. The low-level 'gaia_ptr_t' API excludes all usage of catalog
 * and type metadata.
 */
namespace gaia_ptr
{

gaia_ptr_t create(
    common::gaia_type_t type,
    size_t data_size,
    const void* data);

gaia_ptr_t create(
    common::gaia_id_t id,
    common::gaia_type_t type,
    size_t data_size,
    const void* data);

void remove(gaia_ptr_t& object, bool force = false);

void update_payload(common::gaia_id_t id, size_t data_size, const void* data);
void update_payload(gaia_ptr_t& obj, size_t data_size, const void* data);

bool insert_into_reference_container(common::gaia_id_t parent_id, common::gaia_id_t child_id, common::reference_offset_t parent_anchor_slot);
bool insert_into_reference_container(gaia_ptr_t& parent, common::gaia_id_t child_id, common::reference_offset_t parent_anchor_slot);

bool remove_from_reference_container(common::gaia_id_t parent_id, common::gaia_id_t child_id, common::reference_offset_t parent_anchor_slot);
bool remove_from_reference_container(gaia_ptr_t& parent, common::gaia_id_t child_id, common::reference_offset_t parent_anchor_slot);

bool remove_from_reference_container(common::gaia_id_t child_id, common::reference_offset_t child_anchor_slot);
bool remove_from_reference_container(gaia_ptr_t& child, common::reference_offset_t child_anchor_slot);

bool update_parent_reference(common::gaia_id_t child_id, common::gaia_id_t new_parent_id, common::reference_offset_t parent_offset);
bool update_parent_reference(gaia_ptr_t& child, common::gaia_id_t new_parent_id, common::reference_offset_t parent_offset);

} // namespace gaia_ptr
} // namespace db
} // namespace gaia
