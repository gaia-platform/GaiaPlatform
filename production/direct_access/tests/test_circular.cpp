/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

#include "gaia_internal/common/timer.hpp"
#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_addr_book.h"

using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::addr_book;

using std::string;
using std::thread;
using std::to_string;

using gaia_time_t = gaia::common::timer_t;

class gaia_circular_dependency_test : public db_catalog_test_base_t
{
protected:
    gaia_circular_dependency_test()
        : db_catalog_test_base_t(string("addr_book.ddl")){};
};

TEST_F(gaia_circular_dependency_test, test_circular_refernece)
{
    begin_transaction();

    //    for (gaia_table_t table : gaia_table_t::list())
    //    {
    //       std::cout << table.name() << " " << table.type() << " " << table.gaia_id() << std::endl;
    //    }

    std::cout << "gaia_id_t: " << sizeof(gaia_id_t) << std::endl;
    std::cout << "A_t: " << sizeof(A_t) << std::endl;
    std::cout << "edc_object_t: " << sizeof(edc_object_t<c_gaia_type_A, A_t, internal::A, internal::AT>) << std::endl;
    std::cout << "edc_base_t: " << sizeof(edc_base_t) << std::endl;
    std::cout << "edc_db_t: " << sizeof(edc_db_t) << std::endl;

    A_t a_obj = A_t::get(A_t::insert_row("str1", 2));
    B_t b_obj = B_t::get(B_t::insert_row("str2", 3));

    a_obj.set_b(b_obj.gaia_id());

    //    ASSERT_STREQ(a_obj.b().str_val(), "str2");
    //    ASSERT_EQ(a_obj.b().num_val(), 3);
    //    ASSERT_STREQ(b_obj.a().str_val(), "str1");
    //    ASSERT_EQ(b_obj.a().num_val(), 2);
    //
    //    ASSERT_STREQ(a_obj.b().str_val(), "str2");
    //    ASSERT_EQ(a_obj.b().num_val(), 3);
    //    ASSERT_EQ(a_obj.b().num_val(), 3);
}

template <typename T_type>
void print_bytes(const T_type& input, std::ostream& os = std::cout)
{
    const auto* p = reinterpret_cast<const unsigned char*>(&input);
    os << std::hex << std::showbase;
    os << "[";
    for (unsigned int i = 0; i < sizeof(T_type); ++i)
        os << static_cast<int>(*(p++)) << " ";
    os << "]" << std::endl;
}

TEST_F(gaia_circular_dependency_test, test_object_allocation_price)
{
    begin_transaction();

    constexpr int c_count = 100000;

    gaia_id_t ids[c_count];

    for (int i = 0; i < c_count; ++i)
    {
        A_writer w;
        w.num_val = i;
        w.str_val = to_string(i);
        ids[i] = w.insert_row();
    }

    A_t obj = A_t::get(ids[0]);
    edc_reference_t<A_t> ref(ids[0]);

    print_bytes(ids[0]);
    print_bytes(obj);
    print_bytes(ref);

    A_t stuff1[c_count];
    gaia_time_t::log_function_duration(
        [&ids, &stuff1]() {
            for (int i = 0; i < c_count; ++i)
            {
                stuff1[i] = A_t::get(ids[i]);
            }
        },
        "EDC get");

    A_t stuff2[c_count];
    gaia_time_t::log_function_duration(
        [&ids, &stuff2]() {
            for (int i = 0; i < c_count; ++i)
            {
                stuff2[i] = A_t(ids[i]);
            }
        },
        "EDC constructor");

    gaia_id_t stuff3[c_count];
    gaia_time_t::log_function_duration(
        [&ids, &stuff3]() {
            for (int i = 0; i < c_count; ++i)
            {
                stuff3[i] = ids[i];
            }
        },
        "id");
}
