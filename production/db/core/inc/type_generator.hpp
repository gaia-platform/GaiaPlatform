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
    type_generator_t(common::gaia_type_t type, storage::record_iterator_t&& iterator);

    std::optional<common::gaia_id_t> operator()() final;

private:
    common::gaia_type_t m_type;
    storage::record_iterator_t m_iterator;
    bool m_is_initialized;
};

} // namespace db
} // namespace gaia
