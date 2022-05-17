/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

client_transaction_context_t::~client_transaction_context_t()
{
    clear();
}

client_session_context_t::~client_session_context_t()
{
    clear();
}
