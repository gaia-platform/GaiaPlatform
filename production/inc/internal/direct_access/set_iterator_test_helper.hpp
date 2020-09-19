/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "base_iterator_test_helper.hpp"
#include "gaia_addr_book.h"
#include "gaia_iterators.hpp"

using namespace gaia::addr_book;

namespace gaia
{
namespace direct_access
{

class set_iterator_test_helper_t : public base_iterator_test_helper_t<gaia_iterator_t<employee_t>>
{
public:
    using iterator_t = gaia_iterator_t<employee_t>;
    using record_t = iterator_traits<iterator_t>::value_type;
    using record_list_t = vector<record_t>;

    string get_string(const int index)
    {
        return to_string(index);
    }

    record_list_t insert_records(const int amount)
    {
        employee_writer emp_writer = employee_writer();
        record_list_t list;

        for (int i = 0; i < amount; i++)
        {
            emp_writer.name_first = get_string(i);
            list.push_back(record_t::get(
                emp_writer.insert_row()));
        }
        return list;
    }

    string deref_iter_for_string(iterator_t& iter)
    {
        string str((*iter).name_first());
        return str;
    }

    string deref_and_postinc_iter_for_string(iterator_t& iter)
    {
        string str((*iter++).name_first());
        return str;
    }

    string iter_member_string(iterator_t& iter)
    {
        string str(iter->name_first());
        return str;
    }

    string get_string_from_record(record_t& record)
    {
        string str(record.name_first());
        return str;
    }
};

} // namespace direct_access
} // namespace gaia
