/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <stdint.h>

namespace gaia {
namespace db {
namespace triggers {

enum class event_type_t : uint32_t {
    // Transaction events.
    transaction_begin = 1 << 0,
    transaction_commit = 1 << 1,
    transaction_rollback = 1 << 2,
    // Row events.
    row_update = 1 << 3,
    row_insert = 1 << 4,
    row_delete = 1 << 5,
};

}
}
}
