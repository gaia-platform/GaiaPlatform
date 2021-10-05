/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <optional>

#include "gaia_internal/common/generator_iterator.hpp"

#include "db_internal_types.hpp"
#include "record_list_manager.hpp"

namespace gaia
{
namespace db
{

// 'Movable' type generator
class type_generator_t : public common::iterators::generator_t<common::gaia_id_t>
{
public:
    type_generator_t(common::gaia_type_t type, gaia_txn_id_t txn_id);

    std::optional<common::gaia_id_t> operator()() final;

    type_generator_t(const type_generator_t&) = delete;
    type_generator_t& operator=(const type_generator_t&) = delete;

    ~type_generator_t();

private:
    common::gaia_type_t m_type;
    gaia_txn_id_t m_txn_id;
    storage::record_iterator_t m_iterator;
    bool m_is_initialized;
    // If a snapshot has not been created, we create it.
    // This flag is true if we created the snapshot so we remember to close it.
    bool m_manage_local_snapshot;
};

} // namespace db
} // namespace gaia
