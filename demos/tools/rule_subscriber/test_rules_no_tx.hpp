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
 rule-1: [AddrBook::Employee](update, insert);[AddrBook::Employee.name_last];[AddrBook::Employee.name_first]
  */
void ObjectRule_handler(const rule_context_t*);
}
