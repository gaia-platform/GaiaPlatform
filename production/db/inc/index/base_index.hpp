/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <shared_mutex>
#include <sstream>

#include "gaia/common.hpp"
#include "gaia/exception.hpp"
#include "gaia_internal/db/db_types.hpp"

namespace gaia
{
namespace db
{
namespace index
{

/*
 * Value index types.
 */

enum value_index_type_t : uint8_t
{
    hash,
    range
};

struct index_record_t
{
    gaia::db::gaia_locator_t locator;
    gaia::db::gaia_txn_id_t txn_id;
    gaia::db::gaia_offset_t offset;
    uint8_t deleted;
};

class index_not_found : public common::gaia_exception
{
public:
    explicit index_not_found(const common::gaia_id_t index_id)
    {
        std::stringstream message;
        message << "Cannot find index \"" << index_id << "\".";
        m_message = message.str();
    }
};

class invalid_index_type : public common::gaia_exception
{
public:
    explicit invalid_index_type()
    {
        std::stringstream message;
        message << "Invalid index type. ";
        m_message = message.str();
    }
};

class base_index_t
{
private:
    gaia::common::gaia_id_t index_id;
    value_index_type_t index_type;

protected:
    mutable std::shared_mutex index_lock;

public:
    base_index_t(gaia::common::gaia_id_t index_id, value_index_type_t index_type)
        : index_id(index_id), index_type(index_type)
    {
    }

    gaia::common::gaia_id_t id() const
    {
        return index_id;
    }
    value_index_type_t type() const
    {
        return index_type;
    }

    std::shared_mutex& get_lock() const
    {
        return index_lock;
    }

    bool operator==(const base_index_t& other) const
    {
        return index_id == other.index_id;
    }

    virtual ~base_index_t() = 0;
};

} // namespace index
} // namespace db
} // namespace gaia
