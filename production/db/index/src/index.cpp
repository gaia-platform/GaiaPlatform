/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "index.hpp"

#include "field_access.hpp"

#include "gaia_internal/common/retail_assert.hpp"

using namespace gaia::common;
using namespace gaia::db::payload_types;
using namespace gaia::db::index;

int index_record_t::compare(const index_record_t& other) const
{
    retail_assert(
        key_values.size() == other.key_values.size(),
        "An attempt was made to compare index records of different value.");

    int comparison_result = 0;
    for (size_t i = 0; i < key_values.size(); i++)
    {
        comparison_result = key_values[i].compare(other.key_values[i]);

        // Stop scanning once first difference is encountered.
        if (comparison_result != 0)
        {
            return comparison_result;
        }
    }

    retail_assert(comparison_result == 0, "Internal error: code should have returned already!");

    return comparison_result;
}
