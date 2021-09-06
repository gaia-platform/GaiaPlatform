/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <memory>

#include "gaia/common.hpp"
#include "gaia/exception.hpp"

// Export all symbols declared in this file.
#pragma GCC visibility push(default)

namespace gaia
{

/**
 * \addtogroup Gaia
 * @{
 */

namespace direct_access
{

/**
 * \addtogroup Direct
 * @{
 *
 * Implementation of Extended Data Classes. This provides a direct access API
 * for CRUD operations on the database.
 */

/**
 * Used by iterator class to maintain state of an iteration.
 *
 * This is needed to ensure proper destruction of derived instances via the virtual destructor.
 */
struct edc_base_iterator_state_t
{
    virtual ~edc_base_iterator_state_t() = default;
};

/**
 * Used by edc object, writer, and iterator classes.
 * Not for use outside the context of those classes.
 */
class edc_db_t
{
    friend class edc_base_reference_t;

protected:
    // Low-level interface for iterating over objects of a given container.
    static std::shared_ptr<edc_base_iterator_state_t> initialize_iterator(common::gaia_type_t container_type_id);
    static common::gaia_id_t get_iterator_value(std::shared_ptr<edc_base_iterator_state_t> iterator_state);
    static bool advance_iterator(std::shared_ptr<edc_base_iterator_state_t> iterator_state);

    static common::gaia_id_t get_reference(common::gaia_id_t id, common::reference_offset_t slot);
    static common::gaia_id_t insert(common::gaia_type_t container, size_t data_size, const void* data);
    static void update(common::gaia_id_t id, size_t data_size, const void* data);
    static bool insert_child_reference(common::gaia_id_t parent_id, common::gaia_id_t child_id, common::reference_offset_t child_slot);
    static bool remove_child_reference(common::gaia_id_t parent_id, common::gaia_id_t child_id, common::reference_offset_t child_slot);
    static void delete_row(common::gaia_id_t id);
    static bool get_type(common::gaia_id_t id, common::gaia_type_t& type);
};

/**
 * The edc_base_t class is a tag to mark extended data class objects as well as provide
 * non-template functionality.
 */
class edc_base_t : protected edc_db_t
{
    friend class edc_base_reference_t;

public:
    /**
     * The edc_base_t and edc_object_t shouldn't be instantiated directly. The
     * edc_object_t is created to be subclassed by a "typed" class that is identified
     * with a flatbuffer table. This method returns the name of that type.
     */
    edc_base_t() = default;
    explicit edc_base_t(common::gaia_id_t id);

    virtual common::gaia_type_t gaia_type() = 0;
    virtual ~edc_base_t() = default;

    /**
     * This is the database's identification of this object. The id can be
     * used to refer to this object later.
     */
    common::gaia_id_t gaia_id() const;

protected:
    bool exists() const;

    bool equals(const edc_base_t& other) const;
    const char* data() const;

    virtual common::gaia_id_t* references() const;

    template <typename T_ptr>
    constexpr const T_ptr* to_const_ptr() const;

    template <typename T_ptr>
    constexpr T_ptr* to_ptr();

    void set_record(common::gaia_id_t new_id);

private:
    /**
     * Handle to the underlying database record. This handle should only be accessed via the to_ptr methods.
     */
    common::gaia_handle_t m_record;
};

/**
 * Represent the parent side of a 1:1 relationship. This class is not
 * meant to be used standalone but to be subclassed in the EDC generated
 * code.
 */
class edc_base_reference_t
{
public:
    edc_base_reference_t() = delete;
    edc_base_reference_t(common::gaia_id_t parent, common::reference_offset_t child_offset);
    bool connect(gaia::common::gaia_id_t old_id, gaia::common::gaia_id_t new_id);
    bool disconnect(common::gaia_id_t id);

private:
    common::gaia_id_t m_parent_id;
    common::reference_offset_t m_child_offset;
};

// Exception when get() argument does not match the class type.
class edc_invalid_object_type : public common::gaia_exception
{
public:
    edc_invalid_object_type(
        common::gaia_id_t id,
        common::gaia_type_t expected_type,
        const char* expected_typename,
        common::gaia_type_t actual_type);
};

// A child's parent pointer must match the parent record we have.
class edc_invalid_member : public common::gaia_exception
{
public:
    edc_invalid_member(
        common::gaia_id_t id,
        common::gaia_type_t parent,
        const char* parent_type,
        common::gaia_type_t child,
        const char* child_name);
};

// When a child refers to a parent, but is not found in that parent's list.
class edc_inconsistent_list : public common::gaia_exception
{
public:
    edc_inconsistent_list(
        common::gaia_id_t id,
        const char* parent_type,
        common::gaia_id_t child,
        const char* child_name);
};

// To connect two objects, a gaia_id() is needed but not available until SE create is called during
// the insert_row().
class edc_invalid_state : public common::gaia_exception
{
public:
    edc_invalid_state(
        common::gaia_id_t parent_id,
        common::gaia_id_t chile_id,
        const char* child_type);
};

// An attempt has been made to insert a member that has already been inserted somewhere.
class edc_already_inserted : public common::gaia_exception
{
public:
    edc_already_inserted(common::gaia_id_t parent, const char* parent_type);
};

/*@}*/
} // namespace direct_access
/*@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
