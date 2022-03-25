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
 * @addtogroup gaia
 * @{
 */
namespace direct_access
{
/**
 * @addtogroup direct_access
 * @{
 */

#include "gaia/internal/direct_access/dac_base.inc"

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
     * with a table.
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

    void set(common::gaia_id_t new_id);

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

/**@}*/
} // namespace direct_access
/**@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
