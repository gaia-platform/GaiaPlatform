/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <memory>

#include "gaia/common.hpp"

#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/system_table_types.hpp"
#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/db/gaia_ptr_api.hpp"

#include "memory_types.hpp"

namespace gaia
{
namespace db
{

/**
 * 'gaia_ptr_t' is implemented differently on the server and client-side. See
 * their respective cpp files for differences.
 *
 * Please avoid adding catalog dependent components to this class directly. For
 * higher level funtionalities, add separate functions or classes.
 */
class gaia_ptr_t
{
    friend void gaia_ptr::update_payload(gaia_ptr_t& obj, size_t data_size, const void* data);
    friend gaia_ptr_t gaia_ptr::create(common::gaia_id_t id, common::gaia_type_t type, size_t data_size, const void* data);

public:
    gaia_ptr_t() = default;

    static gaia_ptr_t from_locator(
        gaia_locator_t locator);

    static gaia_ptr_t from_gaia_id(
        common::gaia_id_t id);

    inline bool operator==(const gaia_ptr_t& other) const;
    inline bool operator!=(const gaia_ptr_t& other) const;
    inline bool operator==(const std::nullptr_t) const;
    inline bool operator!=(const std::nullptr_t) const;
    inline explicit operator bool() const;

    static common::gaia_id_t generate_id();

    // NOTE: This method is only used for testing. Please use
    // 'gaia_ptr::create(...)' in user facing code path.
    static gaia_ptr_t create(
        common::gaia_id_t id,
        common::gaia_type_t type,
        common::reference_offset_t num_refs,
        size_t data_size,
        const void* data);

    // NOTE: This method is only used for testing. Please use
    // 'gaia_ptr::update_payload(...)' in user facing code path.
    void update_payload(size_t data_size, const void* data);

    static gaia_ptr_t find_first(common::gaia_type_t type);
    gaia_ptr_t find_next() const;

    inline bool is_null() const;
    inline bool is_ref_anchor() const;

    inline common::gaia_id_t id() const;
    inline common::gaia_type_t type() const;
    inline char* data() const;
    inline size_t data_size() const;
    inline common::gaia_id_t* references() const;
    inline common::reference_offset_t references_count() const;

    db_object_t* to_ptr() const;
    gaia_offset_t to_offset() const;

    /**
     * Returns an iterator representing a server-side cursor over all objects of the given type.
     */
    static common::iterators::generator_iterator_t<gaia_ptr_t> find_all_iterator(
        common::gaia_type_t type);

    /**
     * Returns a range representing a server-side cursor over all objects of the given type.
     */
    static common::iterators::generator_range_t<gaia_ptr_t> find_all_range(
        common::gaia_type_t type);

    void reset();

    gaia_ptr_t set_reference(common::reference_offset_t offset, common::gaia_id_t id);

    gaia_ptr_t set_references(
        common::reference_offset_t offset1, common::gaia_id_t id1,
        common::reference_offset_t offset2, common::gaia_id_t id2,
        common::reference_offset_t offset3 = common::c_invalid_reference_offset,
        common::gaia_id_t id3 = common::c_invalid_gaia_id);

    static gaia_ptr_t create_ref_anchor(
        common::gaia_id_t parent_id,
        common::gaia_id_t first_child_id);

    void finalize_create();

protected:
    void allocate(size_t size);

    void finalize_update(gaia_offset_t old_offset);

    inline bool is(common::gaia_type_t type) const;

    gaia_ptr_t find_next(common::gaia_type_t type) const;

private:
    static gaia_ptr_t create_no_txn(
        common::gaia_id_t id,
        common::gaia_type_t type,
        common::reference_offset_t num_refs,
        size_t data_size,
        const void* data);

    void update_payload_no_txn(size_t data_size, const void* data);

    void clone_no_txn();

    static std::shared_ptr<common::iterators::generator_t<common::gaia_id_t>> get_id_generator_for_type(common::gaia_type_t type);

private:
    gaia_locator_t m_locator{c_invalid_gaia_locator};

    inline explicit gaia_ptr_t(gaia_locator_t locator)
        : m_locator(locator)
    {
    }
};

static_assert(
    sizeof(gaia_ptr_t) == sizeof(gaia_locator_t),
    "gaia_ptr_t shouldn't contain more than a gaia_locator_t value!");

static_assert(
    sizeof(gaia_ptr_t) <= sizeof(common::gaia_handle_t),
    "The size of gaia_handle_t is too small to hold a gaia_ptr_t value!");

class gaia_ptr_generator_t : public common::iterators::generator_t<gaia_ptr_t>
{
public:
    explicit gaia_ptr_generator_t(std::shared_ptr<common::iterators::generator_t<common::gaia_id_t>> id_generator);
    std::optional<gaia_ptr_t> operator()() final;

private:
    std::shared_ptr<common::iterators::generator_t<common::gaia_id_t>> m_id_generator;
};

#include "gaia_ptr.inc"

} // namespace db
} // namespace gaia
