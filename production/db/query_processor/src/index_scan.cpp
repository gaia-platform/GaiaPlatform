/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "index_scan.hpp"

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/catalog_core.hpp"

#include "index_scan_physical.hpp"
#include "predicate.hpp"
#include "qp_operator.hpp"

using namespace gaia::db::index;

namespace gaia
{
namespace db
{
namespace query_processor
{
namespace scan
{

index_scan_iterator_t index_scan_t::begin()
{
    if (m_has_limit)
    {
        return index_scan_iterator_t(m_index_id, scan_state_t(m_predicate, m_limit));
    }
    else
    {
        return index_scan_iterator_t(m_index_id, scan_state_t(m_predicate));
    }
}

std::nullptr_t index_scan_t::end()
{
    return nullptr;
}

index_scan_iterator_t::index_scan_iterator_t(gaia::common::gaia_id_t index_id, scan_state_t scan_state)
    : m_index_id(index_id), m_scan_impl(base_index_scan_physical_t::open(m_index_id, scan_state.predicate())), m_scan_state(std::move(scan_state))
{
    if (m_scan_state.limit_reached())
    {
        m_gaia_ptr = db::gaia_ptr_t();
    }
    else
    {
        // This will result in the index scan init() method being called.
        auto ptr = db::gaia_ptr_t(m_scan_impl->locator());

        // Advance scan to next row passing our filters.
        while (!m_scan_state.should_return_row(ptr))
        {
            m_scan_impl->next_visible_locator();
            ptr = db::gaia_ptr_t(m_scan_impl->locator());
        }
        m_gaia_ptr = ptr;
    }
}

index_scan_iterator_t index_scan_iterator_t::operator++()
{
    if (m_scan_state.limit_reached())
    {
        m_gaia_ptr = db::gaia_ptr_t();
    }
    else
    {
        m_scan_impl->next_visible_locator();
        auto ptr = db::gaia_ptr_t(m_scan_impl->locator());

        // Advance scan to next row passing our filters.
        while (!m_scan_state.should_return_row(ptr))
        {
            m_scan_impl->next_visible_locator();
            ptr = db::gaia_ptr_t(m_scan_impl->locator());
        }

        m_gaia_ptr = ptr;
    }
    return *this;
}

index_scan_iterator_t index_scan_iterator_t::operator++(int)
{
    auto tmp = *this;
    ++(*this);
    return tmp;
}

gaia::db::gaia_ptr_t index_scan_iterator_t::operator*() const
{
    return m_gaia_ptr;
}

gaia::db::gaia_ptr_t* index_scan_iterator_t::operator->()
{
    return &m_gaia_ptr;
}

bool index_scan_iterator_t::operator==(const index_scan_iterator_t& other) const
{
    return m_index_id == other.m_index_id && m_gaia_ptr == other.m_gaia_ptr;
}

bool index_scan_iterator_t::operator!=(const index_scan_iterator_t& other) const
{
    return !(*this == other);
}

bool index_scan_iterator_t::operator==(std::nullptr_t) const
{
    return m_gaia_ptr == nullptr;
}

bool index_scan_iterator_t::operator!=(std::nullptr_t) const
{
    return m_gaia_ptr != nullptr;
}

// Factory method. This method looks up the type of index associated with the index_id
// and returns the appropriate index scan type.
std::shared_ptr<base_index_scan_physical_t>
base_index_scan_physical_t::open(common::gaia_id_t index_id, std::shared_ptr<index_predicate_t> predicate)
{
    db_client_proxy_t::verify_txn_active();
    db_client_proxy_t::rebuild_local_indexes();

    // Get type of index.
    auto it = db::get_indexes()->find(index_id);

    // The index has not been touched this txn. Create an empty index entry.
    if (it == db::get_indexes()->end())
    {
        auto view_ptr = db::id_to_ptr(index_id);

        if (view_ptr == nullptr)
        {
            throw db::index::index_not_found(index_id);
        }

        auto index_view = db::index_view_t(view_ptr);
        it = db::index::index_builder_t::create_empty_index(index_id, index_view);
    }
    auto index = it->second;

    switch (index->type())
    {
    case catalog::index_type_t::range:
        return std::make_shared<range_scan_impl_t>(index_id, predicate);
    case catalog::index_type_t::hash:
        return std::make_shared<hash_scan_impl_t>(index_id, predicate);
    default:
        ASSERT_UNREACHABLE("Index type not found.");
    }

    return nullptr;
}

scan_state_t::scan_state_t(std::shared_ptr<index_predicate_t> predicate)
    : m_predicate(std::move(predicate))
{
    if (m_predicate && m_predicate->query_type() == messages::index_query_t::index_point_read_query_t)
    {
        m_has_limit = true;
        m_limit_rows_remaining = 1;
    }
    else
    {
        m_has_limit = false;
    }
} // namespace scan

scan_state_t::scan_state_t(std::shared_ptr<index_predicate_t> predicate, size_t limit)
    : m_predicate(std::move(predicate)), m_has_limit(true), m_limit_rows_remaining(limit)
{
}

bool scan_state_t::should_return_row(gaia_ptr_t ptr)
{
    if (m_predicate && ptr)
    {
        if (m_predicate->filter(ptr))
        {
            ASSERT_INVARIANT(!limit_reached(), "Limit for scan reached.");
            --m_limit_rows_remaining;
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        ASSERT_INVARIANT(!limit_reached(), "Limit for scan reached.");
        --m_limit_rows_remaining;
        return true;
    }
}

std::shared_ptr<index_predicate_t> scan_state_t::predicate()
{
    return m_predicate;
}

} // namespace scan
} // namespace query_processor
} // namespace db
} // namespace gaia
