////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

template <typename... T_keys>
index_key_t::index_key_t(T_keys... keys)
{
    multi_insert(keys...);
}

template <typename T_key, typename... T_keys>
void index_key_t::multi_insert(T_key key_value, T_keys... rest)
{
    insert(key_value);
    multi_insert(rest...);
}

template <typename T_key>
void index_key_t::multi_insert(T_key key_value)
{
    insert(key_value);
}
