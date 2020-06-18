/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "rules.hpp"
#include "addr_book_gaia_generated.h"
#pragma once
using namespace gaia::rules;

/** ruleset*/
namespace ruleset_1
{
/**
 rule-1: [AddrBook::Employee](update, insert);
 [](commit)
 */
void ObjectRule_handler(const rule_context_t*);

/**
 rule-2: [AddrBook::Employee.name_last]; [AddrBook::Employee.name_first]
 */
void Field_handler(const rule_context_t*);

/**
 rule-3: [AddrBook::Employee](delete)
 */
void Table_handler(const rule_context_t*);

/**
 rule-4: [](commit, rollback)
*/
void TransactionRule_handler(const rule_context_t*);
}
