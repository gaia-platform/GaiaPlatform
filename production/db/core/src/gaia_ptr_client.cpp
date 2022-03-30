/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia/common.hpp"
#include "gaia/exceptions.hpp"

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"
#include "gaia_internal/exceptions.hpp"

#include "db_client.hpp"
#include "db_hash_map.hpp"
#include "db_helpers.hpp"

#ifdef DEBUG
#include "memory_helpers.hpp"
#define WRITE_PROTECT(o) write_protect_allocation_page_for_offset((o))
#else
#define WRITE_PROTECT(o) ((void)0)
#endif

using namespace gaia::common;
using namespace gaia::common::iterators;
using namespace gaia::db;

namespace gaia
{
namespace db
{

/*
 * Client-side implementation of gaia_ptr_t here.
 */

void gaia_ptr_t::reset()
{
    locators_t* locators = get_locators();
    client_t::txn_log(m_locator, to_offset(), c_invalid_gaia_offset);

    // TODO[GAIAPLAT-445]:  We don't expose delete events.
    // if (client_t::is_valid_event(to_ptr()->type))
    // {
    //     client_t::s_events.emplace_back(event_type_t::row_delete, to_ptr()->type, to_ptr()->id, empty_position_list, get_txn_id());
    // }

    (*locators)[m_locator] = c_invalid_gaia_offset;
    m_locator = c_invalid_gaia_locator;
}

db_object_t* gaia_ptr_t::to_ptr() const
{
    client_t::verify_txn_active();
    return locator_to_ptr(m_locator);
}

gaia_offset_t gaia_ptr_t::to_offset() const
{
    client_t::verify_txn_active();
    return locator_to_offset(m_locator);
}

void gaia_ptr_t::finalize_create()
{
    WRITE_PROTECT(to_offset());
    client_t::txn_log(m_locator, c_invalid_gaia_offset, to_offset());
}

void gaia_ptr_t::finalize_update(gaia_offset_t old_offset)
{
    WRITE_PROTECT(to_offset());
    client_t::txn_log(m_locator, old_offset, to_offset());
}

gaia_ptr_t gaia_ptr_t::create(gaia_id_t id, gaia_type_t type, reference_offset_t references_count, size_t data_size, const void* data)
{
    gaia_ptr_t obj = create_no_txn(id, type, references_count, data_size, data);
    obj.finalize_create();
    return obj;
}

void gaia_ptr_t::update_payload(size_t data_size, const void* data)
{
    gaia_offset_t old_offset = to_offset();
    update_payload_no_txn(data_size, data);
    finalize_update(old_offset);
}

gaia_ptr_t gaia_ptr_t::create_ref_anchor(gaia_id_t parent_id, gaia_id_t first_child_id)
{
    gaia_ptr_t obj = create_no_txn(
        generate_id(),
        static_cast<gaia_type_t::value_type>(system_table_type_t::catalog_gaia_ref_anchor),
        c_ref_anchor_ref_num,
        0,
        nullptr);
    obj.references()[c_ref_anchor_parent_offset] = parent_id;
    obj.references()[c_ref_anchor_first_child_offset] = first_child_id;

    obj.finalize_create();
    return obj;
}

gaia_ptr_t gaia_ptr_t::create_no_txn(gaia_id_t id, gaia_type_t type, reference_offset_t references_count, size_t data_size, const void* data)
{
    size_t references_size = references_count * sizeof(gaia_id_t);
    size_t total_payload_size = data_size + references_size;
    if (total_payload_size > c_db_object_max_payload_size)
    {
        throw object_too_large_internal(total_payload_size, c_db_object_max_payload_size);
    }

    // TODO: this constructor allows creating a gaia_ptr_t in an invalid state;
    //  the db_object_t should either be initialized before and passed in
    //  or it should be initialized inside the constructor.
    gaia_locator_t locator = allocate_locator(type);
    hash_node_t* hash_node = db_hash_map::insert(id);
    hash_node->locator = locator;
    allocate_object(locator, total_payload_size);
    gaia_ptr_t obj(locator);
    db_object_t* obj_ptr = obj.to_ptr();
    obj_ptr->id = id;
    obj_ptr->type = type;
    obj_ptr->references_count = references_count;
    if (references_count > 0)
    {
        memset(obj_ptr->payload, 0, references_size);
    }
    obj_ptr->payload_size = total_payload_size;
    if (data)
    {
        memcpy(obj_ptr->payload + references_size, data, data_size);
    }
    else
    {
        ASSERT_INVARIANT(data_size == 0, "Null payload with non-zero payload size!");
    }

    return obj;
}

void gaia_ptr_t::clone_no_txn()
{
    db_object_t* old_this = to_ptr();
    size_t total_payload_size = old_this->payload_size;
    size_t total_object_size = c_db_object_header_size + total_payload_size;
    allocate_object(m_locator, total_payload_size);
    db_object_t* new_this = to_ptr();
    memcpy(new_this, old_this, total_object_size);
}

void gaia_ptr_t::update_payload_no_txn(size_t data_size, const void* data)
{
    db_object_t* old_this = to_ptr();

    size_t references_size = old_this->references_count * sizeof(gaia_id_t);
    size_t total_payload_size = data_size + references_size;
    if (total_payload_size > c_db_object_max_payload_size)
    {
        throw object_too_large_internal(total_payload_size, c_db_object_max_payload_size);
    }

    // Updates m_locator to point to the new object.
    allocate_object(m_locator, total_payload_size);

    db_object_t* new_this = to_ptr();

    memcpy(new_this, old_this, c_db_object_header_size);
    new_this->payload_size = total_payload_size;
    if (old_this->references_count > 0)
    {
        memcpy(new_this->payload, old_this->payload, references_size);
    }
    new_this->references_count = old_this->references_count;
    memcpy(new_this->payload + references_size, data, data_size);
}

gaia_ptr_t gaia_ptr_t::set_reference(reference_offset_t offset, gaia_id_t id)
{
    gaia_offset_t old_offset = to_offset();
    clone_no_txn();
    references()[offset] = id;
    finalize_update(old_offset);
    return *this;
}

gaia_ptr_t gaia_ptr_t::set_references(
    reference_offset_t offset1, gaia_id_t id1,
    reference_offset_t offset2, gaia_id_t id2,
    reference_offset_t offset3, gaia_id_t id3)
{
    ASSERT_PRECONDITION(offset1 != c_invalid_reference_offset, "Unexpected invalid reference offset.");
    ASSERT_PRECONDITION(offset2 != c_invalid_reference_offset, "Unexpected invalid reference offset.");
    gaia_offset_t old_offset = to_offset();
    clone_no_txn();
    references()[offset1] = id1;
    references()[offset2] = id2;
    if (offset3 != c_invalid_reference_offset)
    {
        references()[offset3] = id3;
    }
    finalize_update(old_offset);
    return *this;
}

} // namespace db
} // namespace gaia
