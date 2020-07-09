/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <memory>
#include <cstdint>
#include <vector>

//TEMPORARY defs
typedef uint64_t gaia_id_t;

using namespace std;

const size_t kMaxVectorReserve = 8;

namespace gaia
{
namespace db
{
namespace types
{
    // This structure defines changeset for fields in the table
    class field_changeset_t {
        private:
        gaia_id_t m_table_id;
        // Backing structure for this changeset
        unique_ptr< vector<gaia_id_t> > m_data;
        // Initialize the backing structure
        void initialize();

        public:
        // Constructor, accepts table_id for this changeset as the input.
        field_changeset_t(gaia_id_t table_id);
        // Is there a change in to this specific field?
        bool test(gaia_id_t field_id) const;

        // Number of changed fields in this set.
        size_t size() const;

        // No changes in this changeset?
        bool empty() const;

        // Set/clear individual fields.
        void set(gaia_id_t field_id);
        void clear(gaia_id_t field_id);

        // Validate: check if this changeset is valid against the catalog
        bool validate() const;

        // Getter: table_id
        gaia_id_t get_table_id() const;
    };
}
}
}
