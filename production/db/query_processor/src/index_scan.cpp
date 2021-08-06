/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "index_scan.hpp"

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/catalog_core.hpp"

#include "index_scan_impl.hpp"
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
    return index_scan_iterator_t(m_index_id);
}
std::nullptr_t index_scan_t::end()
{
    return nullptr;
}

index_scan_iterator_t::index_scan_iterator_t(gaia::common::gaia_id_t index_id)
    : m_index_id(index_id), m_scan_impl(base_index_scan_impl_t::get(m_index_id))
{
    m_gaia_ptr = db::gaia_ptr_t(m_scan_impl->locator());
}

index_scan_iterator_t index_scan_iterator_t::operator++()
{
    m_scan_impl->next_visible_locator();
    m_gaia_ptr = db::gaia_ptr_t(m_scan_impl->locator());
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

// Factory for the base impl for index scans.
std::shared_ptr<base_index_scan_impl_t>
base_index_scan_impl_t::get(common::gaia_id_t index_id)
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
        it = db::index::index_builder_t::create_empty_index(index_id, index_view.type());
    }
    auto index = it->second;

    switch (index->type())
    {
    case catalog::index_type_t::range:
        return std::make_shared<range_scan_impl_t>(index_id);
    case catalog::index_type_t::hash:
        return std::make_shared<hash_scan_impl_t>(index_id);
    default:
        ASSERT_UNREACHABLE("Index type not found.");
    }

    return nullptr;
}

} // namespace scan
} // namespace query_processor
} // namespace db
} // namespace gaia
