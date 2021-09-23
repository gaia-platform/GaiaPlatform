/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <mutex>

#include "gaia_internal/common/generator_iterator.hpp"

namespace gaia
{
namespace db
{
namespace index
{
template <typename T_structure>
class index_generator_t : public common::iterators::generator_t<index_record_t>
{
public:
    index_generator_t(std::recursive_mutex& mutex, T_structure& data, gaia_txn_id_t txn_id);
    index_generator_t(
        std::recursive_mutex& mutex,
        typename T_structure::const_iterator begin,
        typename T_structure::const_iterator end,
        gaia_txn_id_t txn_id);

    std::optional<index_record_t> operator()() final;

    void init() override;
    void cleanup() override;

private:
    bool m_initialized;
    std::recursive_mutex& m_index_lock;
    typename T_structure::const_iterator m_it;
    typename T_structure::const_iterator m_end;
    gaia_txn_id_t m_txn_id;
};

#include "index_generator.inc"

} // namespace index
} // namespace db
} // namespace gaia
