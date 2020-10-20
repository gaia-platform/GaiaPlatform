/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <functional>
#include <utility>

#include "base_index.hpp"
#include "generator_iterator.hpp"

namespace gaia
{
namespace db
{
namespace index
{

typedef common::iterators::generator_iterator_t<index_record_t> index_stream_iterator_t;
typedef common::iterators::range_t<index_stream_iterator_t> index_stream_range_t;

class client_index_stream
{
private:
    static int get_record_cursor_socket_for_index(common::gaia_id_t index_id);
    static std::function<std::optional<index_record_t>()> get_record_generator_for_index(common::gaia_id_t index_id);

public:
    static index_stream_iterator_t stream_iterator(common::gaia_id_t index_id);
    static index_stream_range_t stream_iterator_range(common::gaia_id_t index_id);
};

} // namespace index
} // namespace db
} // namespace gaia
