////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <memory>

#include "gaia_internal/db/db_types.hpp"

#include "index.hpp"
#include "predicate.hpp"

namespace gaia
{
namespace db
{
namespace query_processor
{
namespace scan
{

// Class containing generators needed for scans.
class scan_generator_t
{
public:
    static std::shared_ptr<gaia::common::iterators::generator_t<index::index_record_t>>
    get_record_generator_for_index(
        common::gaia_id_t index_id,
        gaia_txn_id_t txn_id,
        std::shared_ptr<query_processor::scan::index_predicate_t> predicate);

private:
    static std::shared_ptr<int> get_record_cursor_socket_for_index(
        common::gaia_id_t index_id,
        gaia_txn_id_t txn_id,
        std::shared_ptr<query_processor::scan::index_predicate_t> predicate);
};

} // namespace scan
} // namespace query_processor
} // namespace db
} // namespace gaia
