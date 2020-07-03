/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <map>

#include "gaia_exception.hpp"
#include "storage_engine.hpp"

using namespace std;
using namespace gaia::db;

namespace gaia {

/**
 * \addtogroup Gaia
 * @{
 */

namespace direct_access {

/**
 * \addtogroup Direct
 * @{
 * 
 * Provides a RAII object for exception safe handling of transactions.
 * 
 */

/**
 * The auto transaction object begins a transaction on construction and 
 * does a rollback on destruction.  The user must explicitly call commit()
 */
class auto_transaction_t
{
public:
    auto_transaction_t();

    void commit(bool auto_begin = true);

    ~auto_transaction_t();
};


/*@}*/
}
/*@}*/
}
