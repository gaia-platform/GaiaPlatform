////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

template <typename T_type, T_type default_invalid_value, typename T_type_constraint>
int_type_t<T_type, default_invalid_value, T_type_constraint>::operator T_type() const
{
    return m_value;
}

template <typename T_type, T_type default_invalid_value, typename T_type_constraint>
T_type&
int_type_t<T_type, default_invalid_value, T_type_constraint>::value_ref()
{
    return m_value;
}

template <typename T_type, T_type default_invalid_value, typename T_type_constraint>
int_type_t<T_type, default_invalid_value, T_type_constraint>&
int_type_t<T_type, default_invalid_value, T_type_constraint>::operator+=(
    const int_type_t<T_type, default_invalid_value, T_type_constraint>& other)
{
    this->m_value += other.m_value;
    return *this;
}

template <typename T_type, T_type default_invalid_value, typename T_type_constraint>
int_type_t<T_type, default_invalid_value, T_type_constraint>&
int_type_t<T_type, default_invalid_value, T_type_constraint>::operator-=(
    const int_type_t<T_type, default_invalid_value, T_type_constraint>& other)
{
    this->m_value -= other.m_value;
    return *this;
}

template <typename T_type, T_type default_invalid_value, typename T_type_constraint>
int_type_t<T_type, default_invalid_value, T_type_constraint>&
int_type_t<T_type, default_invalid_value, T_type_constraint>::operator*=(
    const int_type_t<T_type, default_invalid_value, T_type_constraint>& other)
{
    this->m_value *= other.m_value;
    return *this;
}

template <typename T_type, T_type default_invalid_value, typename T_type_constraint>
int_type_t<T_type, default_invalid_value, T_type_constraint>&
int_type_t<T_type, default_invalid_value, T_type_constraint>::operator/=(
    const int_type_t<T_type, default_invalid_value, T_type_constraint>& other)
{
    this->m_value /= other.m_value;
    return *this;
}

template <typename T_type, T_type default_invalid_value, typename T_type_constraint>
int_type_t<T_type, default_invalid_value, T_type_constraint>&
int_type_t<T_type, default_invalid_value, T_type_constraint>::operator%=(
    const int_type_t<T_type, default_invalid_value, T_type_constraint>& other)
{
    this->m_value %= other.m_value;
    return *this;
}

template <typename T_type, T_type default_invalid_value, typename T_type_constraint>
int_type_t<T_type, default_invalid_value, T_type_constraint>&
int_type_t<T_type, default_invalid_value, T_type_constraint>::operator++()
{
    this->m_value += 1;
    return *this;
}

template <typename T_type, T_type default_invalid_value, typename T_type_constraint>
int_type_t<T_type, default_invalid_value, T_type_constraint>
int_type_t<T_type, default_invalid_value, T_type_constraint>::operator++(int)
{
    int_type_t pre_value = *this;
    this->m_value += 1;
    return pre_value;
}

template <typename T_type, T_type default_invalid_value, typename T_type_constraint>
int_type_t<T_type, default_invalid_value, T_type_constraint>&
int_type_t<T_type, default_invalid_value, T_type_constraint>::operator--()
{
    this->m_value -= 1;
    return *this;
}

template <typename T_type, T_type default_invalid_value, typename T_type_constraint>
int_type_t<T_type, default_invalid_value, T_type_constraint>
int_type_t<T_type, default_invalid_value, T_type_constraint>::operator--(int)
{
    int_type_t pre_value = *this;
    this->m_value -= 1;
    return pre_value;
}

// Explicit instantiations of the int_type_t template.
// These are all the instantiations that we support
// and the only ones that we should need.
template class int_type_t<uint64_t, 0>;
template class int_type_t<uint64_t, std::numeric_limits<uint64_t>::max()>;

static_assert(
    sizeof(int_type_t<uint64_t, 0>) == sizeof(uint64_t),
    "The int_type_t<uint64_t> instantiation has a different size than its underlying uint64_t type!");
static_assert(
    sizeof(int_type_t<uint64_t, std::numeric_limits<uint64_t>::max()>) == sizeof(uint64_t),
    "The int_type_t<uint64_t> instantiation has a different size than its underlying uint64_t type!");

template class int_type_t<uint32_t, 0>;
template class int_type_t<uint32_t, std::numeric_limits<uint32_t>::max()>;

static_assert(
    sizeof(int_type_t<uint32_t, 0>) == sizeof(uint32_t),
    "The int_type_t<uint32_t> instantiation has a different size than its underlying uint32_t type!");
static_assert(
    sizeof(int_type_t<uint32_t, std::numeric_limits<uint32_t>::max()>) == sizeof(uint32_t),
    "The int_type_t<uint32_t> instantiation has a different size than its underlying uint32_t type!");

template class int_type_t<uint16_t, 0>;
template class int_type_t<uint16_t, std::numeric_limits<uint16_t>::max()>;

static_assert(
    sizeof(int_type_t<uint16_t, 0>) == sizeof(uint16_t),
    "The int_type_t<uint16_t> instantiation has a different size than its underlying uint16_t type!");
static_assert(
    sizeof(int_type_t<uint16_t, std::numeric_limits<uint16_t>::max()>) == sizeof(uint16_t),
    "The int_type_t<uint16_t> instantiation has a different size than its underlying uint16_t type!");

template class int_type_t<uint8_t, 0>;
template class int_type_t<uint8_t, std::numeric_limits<uint8_t>::max()>;

static_assert(
    sizeof(int_type_t<uint8_t, 0>) == sizeof(uint8_t),
    "The int_type_t<uint8_t> instantiation has a different size than its underlying uint8_t type!");
static_assert(
    sizeof(int_type_t<uint8_t, std::numeric_limits<uint8_t>::max()>) == sizeof(uint8_t),
    "The int_type_t<uint8_t> instantiation has a different size than its underlying uint8_t type!");

template class int_type_t<int64_t, 0>;
template class int_type_t<int64_t, std::numeric_limits<int64_t>::max()>;

static_assert(
    sizeof(int_type_t<int64_t, 0>) == sizeof(int64_t),
    "The int_type_t<int64_t> instantiation has a different size than its underlying int64_t type!");
static_assert(
    sizeof(int_type_t<int64_t, std::numeric_limits<int64_t>::max()>) == sizeof(int64_t),
    "The int_type_t<int64_t> instantiation has a different size than its underlying int64_t type!");

template class int_type_t<int32_t, 0>;
template class int_type_t<int32_t, std::numeric_limits<int32_t>::max()>;

static_assert(
    sizeof(int_type_t<int32_t, 0>) == sizeof(int32_t),
    "The int_type_t<int32_t> instantiation has a different size than its underlying int32_t type!");
static_assert(
    sizeof(int_type_t<int32_t, std::numeric_limits<int32_t>::max()>) == sizeof(int32_t),
    "The int_type_t<int32_t> instantiation has a different size than its underlying int32_t type!");

template class int_type_t<int16_t, 0>;
template class int_type_t<int16_t, std::numeric_limits<int16_t>::max()>;

static_assert(
    sizeof(int_type_t<int16_t, 0>) == sizeof(int16_t),
    "The int_type_t<int16_t> instantiation has a different size than its underlying int16_t type!");
static_assert(
    sizeof(int_type_t<int16_t, std::numeric_limits<int16_t>::max()>) == sizeof(int16_t),
    "The int_type_t<int16_t> instantiation has a different size than its underlying int16_t type!");

template class int_type_t<int8_t, 0>;
template class int_type_t<int8_t, std::numeric_limits<int8_t>::max()>;

static_assert(
    sizeof(int_type_t<int8_t, 0>) == sizeof(int8_t),
    "The int_type_t<int8_t> instantiation has a different size than its underlying int8_t type!");
static_assert(
    sizeof(int_type_t<int8_t, std::numeric_limits<int8_t>::max()>) == sizeof(int8_t),
    "The int_type_t<int8_t> instantiation has a different size than its underlying int8_t type!");
