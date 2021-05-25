/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <map>
#include <memory>

#include "gaia/common.hpp"
#include "gaia/exception.hpp"

namespace gaia
{

/**
 * \addtogroup Gaia
 * @{
 */

namespace direct_access
{

/**
 * Used by iterator class to maintain state of an iteration.
 *
 * This is just a pointer wrapper, to isolate callers from the internal implementation details.
 */
class edc_db_t;
class edc_iterator_state_t
{
    friend class edc_db_t;

public:
    bool is_set();

private:
    // The iterator state.
    std::shared_ptr<uint8_t[]> m_state;
};

/**
 * Used by edc object, writer, and iterator classes.
 * Not for use outside the context of those classes.
 */
class edc_db_t
{
protected:
    // Low-level interface for iterating over objects of a given container.
    static bool initialize_iterator(common::gaia_type_t container_type_id, edc_iterator_state_t& iterator_state);
    static common::gaia_id_t get_iterator_value(edc_iterator_state_t& iterator_state);
    static bool advance_iterator(edc_iterator_state_t& iterator_state);

    static common::gaia_id_t get_reference(common::gaia_id_t id, size_t slot);
    static common::gaia_id_t insert(common::gaia_type_t container, size_t data_size, const void* data);
    static void update(common::gaia_id_t id, size_t data_size, const void* data);
    static void insert_child_reference(common::gaia_id_t parent_id, common::gaia_id_t child_id, size_t child_slot);
    static void remove_child_reference(common::gaia_id_t parent_id, common::gaia_id_t child_id, size_t child_slot);
    static void delete_row(common::gaia_id_t id);
    static bool get_type(common::gaia_id_t id, common::gaia_type_t& type);

private:
    // Internal low-level interface for iterating over the objects of a given container.
    // This interface uses the locator table, which makes it less efficient because
    // it has to iterate over all entries in the database.
    //
    // This is private because this should not be called directly even by classes that inherit from edc_db_t.
    static common::gaia_id_t find_first(common::gaia_type_t container);
    static common::gaia_id_t find_next(common::gaia_id_t id);
};

/**
 * \addtogroup Direct
 * @{
 *
 * Implementation of Extended Data Classes. This provides a direct access API
 * for CRUD operations on the database.
 */

/**
 * The edc_base_t class is a tag to mark extended data class objects as well as provide
 * non-template functionality.
 */
class edc_base_t : protected edc_db_t
{
public:
    /**
     * The edc_base_t and edc_object_t shouldn't be instantiated directly. The
     * edc_object_t is created to be subclassed by a "typed" class that is identified
     * with a flatbuffer table. This method returns the name of that type.
     */
    edc_base_t() = delete;
    explicit edc_base_t(const char* gaia_typename);
    edc_base_t(const char* gaia_typename, common::gaia_id_t id);

    const char* gaia_typename()
    {
        return m_typename;
    }
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

protected:
    /**
     * The type name for the gaia_type generated by gaiac
     */
    const char* m_typename;

    /**
     * Handle to the underlying database record. This handle should only be accessed via the to_ptr methods.
     */
    common::gaia_handle_t m_record;
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
