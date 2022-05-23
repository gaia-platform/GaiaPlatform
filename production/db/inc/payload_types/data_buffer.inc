////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

template <typename T_value>
data_write_buffer_t& data_write_buffer_t::operator<<(T_value value)
{
    write(reinterpret_cast<const char*>(&value), sizeof(T_value));
    return *this;
}

template <typename T_value>
data_read_buffer_t& data_read_buffer_t::operator>>(T_value& value)
{
    ::memcpy(&value, read(sizeof(T_value)), sizeof(T_value));
    return *this;
}
