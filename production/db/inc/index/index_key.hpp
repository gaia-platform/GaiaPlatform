/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <ostream>
#include <vector>

#include "data_holder.hpp"
namespace gaia
{
namespace db
{
namespace index
{

/**
* Reflection based logical key for the index.
*/
class index_key_t
{
    friend std::ostream& operator<<(std::ostream& os, const index_key_t& key);
    friend struct index_key_hash;

public:
    index_key_t() = default;

    template <typename... T_keys>
    index_key_t(T_keys... keys);

    template <typename T_key, typename... T_keys>
    void multi_insert(T_key key_value, T_keys... rest);

    template <typename T_key>
    void multi_insert(T_key key_value);

    void insert(gaia::db::payload_types::data_holder_t value);

    bool operator<(const index_key_t& other) const;
    bool operator<=(const index_key_t& other) const;
    bool operator>(const index_key_t& other) const;
    bool operator>=(const index_key_t& other) const;
    bool operator==(const index_key_t& other) const;

    const std::vector<gaia::db::payload_types::data_holder_t>& values() const;
    size_t size() const;

private:
    int compare(const index_key_t& other) const;

private:
    std::vector<gaia::db::payload_types::data_holder_t> m_key_values;
};

/**
* Standard conforming hash function for index keys.
*/
struct index_key_hash
{
    gaia::db::payload_types::data_hash_t operator()(const index_key_t& key) const;
};

#include "index_key.inc"

} // namespace index
} // namespace db
} // namespace gaia
