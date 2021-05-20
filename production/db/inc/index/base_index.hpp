/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <optional>
#include <ostream>
#include <sstream>

#include "gaia/common.hpp"
#include "gaia/exception.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/db/db_types.hpp"

namespace gaia
{
namespace db
{
namespace index
{

typedef catalog::index_type_t index_type_t;

struct index_record_t
{
    gaia::db::gaia_locator_t locator;
    gaia::db::gaia_txn_id_t txn_id;
    gaia::db::gaia_offset_t offset;
    uint8_t deleted;

    friend std::ostream& operator<<(std::ostream& os, const index_record_t& rec);
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
    index_type_t index_type;

protected:
    mutable std::recursive_mutex index_lock;

public:
    base_index_t(gaia::common::gaia_id_t index_id, index_type_t index_type)
        : index_id(index_id), index_type(index_type)
    {
    }

    gaia::common::gaia_id_t id() const;
    index_type_t type() const;

    std::recursive_mutex& get_lock() const;

    bool operator==(const base_index_t& other) const;

    virtual void clear() = 0;
    virtual std::function<std::optional<index_record_t>()> generator(gaia_txn_id_t txn_id) = 0;
    virtual ~base_index_t() = default;
}; // namespace index

} // namespace index
} // namespace db
} // namespace gaia
