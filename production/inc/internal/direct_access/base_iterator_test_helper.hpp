/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <type_traits>
#include <vector>

namespace gaia
{
namespace direct_access
{

template<typename T_iterator>
class base_iterator_test_helper_t
{
public:
    using iterator_t = T_iterator;
    using record_t = typename iterator_traits<iterator_t>::value_type;
    using record_list_t = vector<record_t>;

    virtual string get_string(const int index) = 0;

    virtual record_list_t insert_records(const int amount) = 0;

    virtual string deref_iter_for_string(iterator_t& iter) = 0;

    virtual string deref_and_postinc_iter_for_string(iterator_t& iter) = 0;

    virtual string iter_member_string(iterator_t& iter) = 0;

    virtual string get_string_from_record(record_t& record) = 0;
};

} // namespace direct_access
} // namespace gaia
