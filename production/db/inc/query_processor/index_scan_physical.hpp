/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <functional>
#include <memory>

#include "gaia/common.hpp"

#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/db/index_builder.hpp"

#include "db_client.hpp"
#include "db_helpers.hpp"
#include "hash_index.hpp"
#include "index.hpp"
#include "qp_operator.hpp"
#include "range_index.hpp"
#include "scan_generators.hpp"

namespace gaia
{
namespace db
{
namespace query_processor
{
namespace scan
{

typedef common::iterators::generator_iterator_t<db::index::index_record_t> remote_scan_iterator_t;

class index_scan_iterator_t;

/**
 * Polymorphic index scan.
 * **/
class base_index_scan_physical_t : public physical_operator_t
{
    friend class gaia::db::query_processor::scan::index_scan_iterator_t;

public:
    base_index_scan_physical_t() = default;
    explicit base_index_scan_physical_t(common::gaia_id_t);

    static std::shared_ptr<base_index_scan_physical_t> open(common::gaia_id_t index_id, std::shared_ptr<index_predicate_t> predicate);
    virtual ~base_index_scan_physical_t() = default;

protected:
    static bool is_visible(const db::index::index_record_t&);

    virtual void next_visible_locator() = 0;
    virtual bool has_more() = 0;
    virtual db::gaia_locator_t locator() = 0;
};

/**
 * Templated implementation.
 *
 * This class contains the actual implementation details of the index scan.
 *
 * It contains:
 * - The RPC calls to the server for initialization.
 * - Merge operation between local and remote.
 * - Visibility resolution of locators.
 *
 * */
template <typename T_index, typename T_index_iterator>
class index_scan_physical_t : public base_index_scan_physical_t
{
public:
    index_scan_physical_t() = default;
    index_scan_physical_t(common::gaia_id_t index_id, std::shared_ptr<index_predicate_t> predicate)
        : m_index_id(index_id), m_initialized(false), m_predicate(std::move(predicate))
    {
    }

    ~index_scan_physical_t() override = default;

protected:
    void next_visible_locator() override;
    bool has_more() override;
    db::gaia_locator_t locator() override;

private:
    db::index::db_index_t m_index;
    T_index_iterator m_local_it;
    T_index_iterator m_local_it_end;
    remote_scan_iterator_t m_remote_it;

    common::gaia_id_t m_index_id;
    db::gaia_locator_t m_locator;
    bool m_initialized;
    std::shared_ptr<index_predicate_t> m_predicate;

    void init();
    db::index::index_key_t record_to_key(const db::index::index_record_t& record) const;
    bool should_merge_local() const;
    void advance_remote();
    void advance_local();
    bool remote_end() const;
    bool local_end() const;
};

typedef index_scan_physical_t<db::index::range_index_t, db::index::range_index_iterator_t> range_scan_impl_t;
typedef index_scan_physical_t<db::index::hash_index_t, db::index::hash_index_iterator_t> hash_scan_impl_t;

#include "index_scan_physical.inc"

} // namespace scan
} // namespace query_processor
} // namespace db
} // namespace gaia
