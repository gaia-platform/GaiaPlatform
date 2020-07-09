#include "field_changeset.hpp"
#include "retail_assert.hpp"

#include <algorithm>

using namespace gaia::db::types;
using namespace gaia::common;
using namespace std;

field_changeset_t::field_changeset_t(gaia_id_t table_id) :
m_table_id(table_id),
m_data(nullptr) {}

// Initialize backing structure on this changeset.
void field_changeset_t::initialize() {
    retail_assert(m_data == nullptr, "field changeset already initialized");
    // TODO(yiwen): lookup catalog for actual size of table
    size_t reserve_size = kMaxVectorReserve;
    m_data.reset(new vector<gaia_id_t>()); // change to make_unique with C++14 and above.
    m_data->reserve(reserve_size);
}

// Set a change in the field_changeset.
void field_changeset_t::set(gaia_id_t field_id) {
    if (!m_data)
        initialize();

    auto it = std::find(m_data->cbegin(), m_data->cend(), field_id);

    if (it == m_data->cend())
        m_data->push_back(field_id);
}

// Clear a change from the field_changeset.
void field_changeset_t::clear(gaia_id_t field_id) {
    if (m_data) {
        auto it = std::find(m_data->begin(), m_data->end(), field_id);
        if (it != m_data->end()) {
            *it = m_data->back();
            m_data->pop_back();
        }
    }
}

// Test if field has changed.
bool field_changeset_t::test(gaia_id_t field_id) const{
    if (m_data) {
        auto it = std::find(m_data->cbegin(), m_data->cend(), field_id);
        return it != m_data->end();
    }
    return false;
}

// Number of changes in this changeset.
size_t field_changeset_t::size() const {
    if (m_data) {
        return m_data->size();
    }
    return 0;
}

// No changes in this changeset?
bool field_changeset_t::empty() const {
    return size() == 0;
}

// Check if changeset is valid.
bool field_changeset_t::validate() const {
    // TODO (yiwen): implement
    return true;
}

// Get table_id associated with this changeset
gaia_id_t field_changeset_t::get_table_id() const {
    return m_table_id;
}
