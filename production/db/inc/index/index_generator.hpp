/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <mutex>
#include <utility>

#include "gaia_internal/common/generator_iterator.hpp"

#include "index_key.hpp"

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
    // Constructor for the index generator.
    // The generator will generate values between begin & end key inclusive.
    // Empty index keys are used to represent the lack of lower/upper bound because
    // they cannot represent a valid indexed key. This will eliminate any ambiguity
    // as to whether those keys represent a specific index value or a lack of value.
    index_generator_t(
        std::recursive_mutex& mutex,
        const T_structure& data,
        const index_key_t& begin_key,
        const index_key_t& end_key,
        gaia_txn_id_t txn_id);

    std::optional<index_record_t> operator()() final;

    void cleanup() override;

private:
    std::pair<typename T_structure::const_iterator, typename T_structure::const_iterator>
    range() const;

    bool m_initialized;
    std::recursive_mutex& m_index_lock;
    const T_structure& m_data;
    index_key_t m_begin_key;
    index_key_t m_end_key;
    gaia_txn_id_t m_txn_id;
    typename T_structure::const_iterator m_it;
    typename T_structure::const_iterator m_end;
};

#include "index_generator.inc"

} // namespace index
} // namespace db
} // namespace gaia
