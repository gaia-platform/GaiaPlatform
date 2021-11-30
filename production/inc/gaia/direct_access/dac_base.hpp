/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <memory>

#include "gaia/common.hpp"

// Export all symbols declared in this file.
#pragma GCC visibility push(default)

namespace gaia
{
/**
 * \addtogroup gaia
 * @{
 */
namespace direct_access
{
/**
 * \addtogroup direct_access
 * @{
 *
 * Implementation of Direct Access Classes. This provides a direct access API
 * for CRUD operations on the database.
 */

/**
 * Used by iterator class to maintain state of an iteration.
 *
 * This is needed to ensure proper destruction of derived instances via the virtual destructor.
 */
struct dac_base_iterator_state_t
{
    virtual ~dac_base_iterator_state_t() = default;
};

/**
 * Used by edc object, writer, and iterator classes.
 * Not for use outside the context of those classes.
 */
class dac_db_t
{
    friend class dac_base_reference_t;

protected:
    // Low-level interface for iterating over objects of a given container.
    static std::shared_ptr<dac_base_iterator_state_t> initialize_iterator(common::gaia_type_t container_type_id);
    static common::gaia_id_t get_iterator_value(std::shared_ptr<dac_base_iterator_state_t> iterator_state);
    static bool advance_iterator(std::shared_ptr<dac_base_iterator_state_t> iterator_state);

    static common::gaia_id_t get_reference(common::gaia_id_t id, common::reference_offset_t slot);
    static common::gaia_id_t insert(common::gaia_type_t container, size_t data_size, const void* data);
    static void update(common::gaia_id_t id, size_t data_size, const void* data);
    static bool insert_child_reference(common::gaia_id_t parent_id, common::gaia_id_t child_id, common::reference_offset_t child_slot);
    static bool remove_child_reference(common::gaia_id_t parent_id, common::gaia_id_t child_id, common::reference_offset_t child_slot);
    static void delete_row(common::gaia_id_t id);
    static bool get_type(common::gaia_id_t id, common::gaia_type_t& type);
};

/**
 * The dac_base_t class is a tag to mark direct access class objects as well as provide
 * non-template functionality.
 */
class dac_base_t : protected dac_db_t
{
    friend class dac_base_reference_t;

public:
    /**
     * The dac_base_t and dac_object_t shouldn't be instantiated directly. The
     * dac_object_t is created to be subclassed by a "typed" class that is identified
     * with a flatbuffer table. This method returns the name of that type.
     */
    dac_base_t() = default;
    explicit dac_base_t(common::gaia_id_t id);

    virtual common::gaia_type_t gaia_type() = 0;
    virtual ~dac_base_t() = default;

    /**
     * This is the database's identification of this object. The id can be
     * used to refer to this object later.
     */
    common::gaia_id_t gaia_id() const;

protected:
    bool exists() const;

    bool equals(const dac_base_t& other) const;
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
 * meant to be used standalone but to be subclassed in the DAC generated
 * code.
 */
class dac_base_reference_t
{
public:
    dac_base_reference_t() = delete;
    dac_base_reference_t(common::gaia_id_t parent, common::reference_offset_t child_offset);
    bool connect(gaia::common::gaia_id_t old_id, gaia::common::gaia_id_t new_id);
    bool disconnect(common::gaia_id_t id);

private:
    common::gaia_id_t m_parent_id;
    common::reference_offset_t m_child_offset;
};

/**
 * @brief For internal use only.
 */
void report_invalid_object_id(common::gaia_id_t id);
void report_invalid_object_type(
    common::gaia_id_t id,
    common::gaia_type_t expected_type,
    const char* expected_typename,
    common::gaia_type_t actual_type);
void report_invalid_object_state(
    common::gaia_id_t parent_id,
    common::gaia_id_t chile_id,
    const char* child_type);

/*@}*/
} // namespace direct_access
/*@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
