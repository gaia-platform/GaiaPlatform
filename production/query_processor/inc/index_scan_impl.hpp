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
#include "range_index.hpp"

namespace gaia
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
class base_index_scan_impl_t : public physical_operator_t
{
    friend class gaia::query_processor::scan::index_scan_iterator_t;

public:
    base_index_scan_impl_t() = default;
    explicit base_index_scan_impl_t(common::gaia_id_t);

    static std::shared_ptr<base_index_scan_impl_t> get(common::gaia_id_t index_id);
    virtual ~base_index_scan_impl_t() = default;

protected:
    static bool is_visible(const db::index::index_record_t&);

    virtual void next_visible_locator() = 0;
    virtual bool has_more() = 0;
    virtual db::gaia_locator_t locator() = 0;
};

/**
 * Templated implementation.
 * */

template <typename T_index, typename T_iter>
class index_scan_impl_t : public base_index_scan_impl_t
{
public:
    index_scan_impl_t() = default;
    explicit index_scan_impl_t(common::gaia_id_t index_id)
        : m_index_id(index_id), m_initialized(false)
    {
    }

    ~index_scan_impl_t() override = default;

protected:
    void next_visible_locator() override;
    bool has_more() override;
    db::gaia_locator_t locator() override;

private:
    db::index::db_index_t m_index;
    T_iter m_local_iter;
    T_iter m_local_iter_end;
    remote_scan_iterator_t m_remote_iter;

    common::gaia_id_t m_index_id;
    db::gaia_locator_t m_locator;
    bool m_initialized;

    void init();
    db::index::index_key_t record_to_key(const db::index::index_record_t& record) const;
    bool select_local_for_merge() const;
    void advance_remote();
    void advance_local();
    bool remote_end() const;
    bool local_end() const;
};

typedef index_scan_impl_t<db::index::range_index_t, db::index::range_index_iterator_t> range_scan_impl_t;
typedef index_scan_impl_t<db::index::hash_index_t, db::index::hash_index_iterator_t> hash_scan_impl_t;

#include "index_scan_impl.inc"

} // namespace scan
} // namespace query_processor
} // namespace gaia
