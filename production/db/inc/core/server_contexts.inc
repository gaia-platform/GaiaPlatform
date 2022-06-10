////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

server_transaction_context_t::~server_transaction_context_t()
{
    clear();
}

server_session_context_t::~server_session_context_t()
{
    clear();
}
