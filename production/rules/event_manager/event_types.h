/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "gaia_direct.h"
#include <vector>

/**
 * Event types that the Gaia Platform supports.
 * Currently a flat structure.  May need to be hierarchical.
 */
enum class event_type {
    on_tx_begin,
    on_tx_commit,
    on_tx_rollback,
    on_col_change, /**< single column value changed */
    on_row_change, /**< more than one column value changed */
    on_row_insert,
    on_row_delete
};

class on_row_change 
{
public:
    gaia_api::gaia_record rows; /**< record pointer to the row that changed */
    std::vector<gaia_api::gaia_field *> columns; /**< field pointer to changed fields */
    gaia_api::gaia_base old_values;   /**< old object */
    gaia_api::gaia_base new_values;   /**< new object */ 
};


class on_col_change 
{
public:
    gaia_api::gaia_record row_source; /**< record pointer to the row that changed */
    gaia_api::gaia_field col_source;  /**< record pointer to the row that changed */
    gaia_api::gaia_base old_values;   /**< old object */
    gaia_api::gaia_base new_values;   /**< new object */ 
};

class on_tx_commit 
{
};

class on_row_insert 
{
public:
    gaia_api::gaia_record row_source; /**< record pointer to the row that got inserted */    
};
