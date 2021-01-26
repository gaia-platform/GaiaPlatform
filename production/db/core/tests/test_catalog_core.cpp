/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include <algorithm>

#include "gaia_internal/db/catalog_core.hpp"
#include "gaia_internal/db/db_test_base.hpp"
#include "gtest/gtest.h"

#include "gaia/common.hpp"
#include "gaia/db/catalog.hpp"
#include "gaia/db/db.hpp"

class catalog_core_test : public db_test_base_t
{
};

TEST_F(catalog_core_test, get_table)
{
    const char test_table_name[] = "get_table_test";
    gaia::catalog::ddl::field_def_list_t fields;
    gaia_id_t table_id = gaia::catalog::create_table(test_table_name, fields);

    begin_transaction();
    auto table_view = gaia::db::catalog_core_t::get_table(table_id);
    ASSERT_STREQ(table_view.name(), test_table_name);
    commit_transaction();
}

TEST_F(catalog_core_test, list_tables)
{
    std::set<gaia_type_t> system_table_ids{
        static_cast<gaia_type_t>(system_table_type_t::catalog_gaia_field),
        static_cast<gaia_type_t>(system_table_type_t::catalog_gaia_table),
        static_cast<gaia_type_t>(system_table_type_t::catalog_gaia_rule),
        static_cast<gaia_type_t>(system_table_type_t::catalog_gaia_ruleset),
        static_cast<gaia_type_t>(system_table_type_t::catalog_gaia_database),
        static_cast<gaia_type_t>(system_table_type_t::catalog_gaia_relationship),
        static_cast<gaia_type_t>(system_table_type_t::event_log),
    };

    gaia::catalog::initialize_catalog();

    std::set<gaia_type_t> list_result;
    begin_transaction();
    for (auto table_view : gaia::db::catalog_core_t::list_tables())
    {
        list_result.insert(table_view.table_type());
    }
    commit_transaction();

    ASSERT_TRUE(std::includes(list_result.begin(), list_result.end(), system_table_ids.begin(), system_table_ids.end()));
}

TEST_F(catalog_core_test, list_fields)
{
    const char test_table_name[] = "list_fields_test";
    gaia::catalog::ddl::field_def_list_t fields;
    fields.emplace_back(std::make_unique<gaia::catalog::ddl::data_field_def_t>("id", data_type_t::e_int8, 1));
    fields.emplace_back(std::make_unique<gaia::catalog::ddl::data_field_def_t>("name", data_type_t::e_string, 1));

    gaia_id_t table_id = gaia::catalog::create_table(test_table_name, fields);

    std::vector<field_view_t> field_views;
    begin_transaction();
    for (auto field_view : catalog_core_t::list_fields(table_id))
    {
        field_views.push_back(field_view);
    }
    commit_transaction();

    ASSERT_TRUE(std::equal(
        field_views.begin(), field_views.end(), fields.rbegin(), fields.rend(),
        [](auto& field_view, auto& field_def) -> bool {
            auto data_field_def = dynamic_cast<gaia::catalog::ddl::data_field_def_t*>(field_def.get());
            if (field_view.name() == data_field_def->name && field_view.data_type() == data_field_def->data_type)
            {
                return true;
            }
            return false;
        }));
}

TEST_F(catalog_core_test, list_relationship_from)
{
    // CREATE TABLE star(name STRING, orbit REFERENCES star);
    // CREATE TABLE planet(name STRING, orbit REFERENCES star);
    // CREATE TABLE comet(name STRING, orbit REFERENCES star);
    gaia::catalog::ddl::field_def_list_t fields;
    fields.emplace_back(std::make_unique<gaia::catalog::ddl::data_field_def_t>("name", data_type_t::e_string, 1));
    fields.emplace_back(std::make_unique<gaia::catalog::ddl::ref_field_def_t>("orbit", "", "star"));

    auto star_table_id = gaia::catalog::create_table("star", fields);
    auto planet_table_id = gaia::catalog::create_table("planet", fields);
    auto comet_table_id = gaia::catalog::create_table("comet", fields);

    std::set<gaia_id_t> tables_with_relationship_from_star;
    begin_transaction();
    for (relationship_view_t relationship : catalog_core_t::list_relationship_from(star_table_id))
    {
        tables_with_relationship_from_star.insert(relationship.child_table_id());
    }
    commit_transaction();
    ASSERT_EQ(tables_with_relationship_from_star, std::set({star_table_id, planet_table_id, comet_table_id}));
}

TEST_F(catalog_core_test, list_relationship_to)
{
    // CREATE TABLE color(name STRING);
    // CREATE TABLE shape(name STRING);
    // CREATE TABLE object(name STRING, color REFERENCES color, shape REFERENCES shape);
    gaia::catalog::ddl::field_def_list_t fields;
    fields.emplace_back(std::make_unique<gaia::catalog::ddl::data_field_def_t>("name", data_type_t::e_string, 1));

    auto color_table_id = gaia::catalog::create_table("color", fields);
    auto shape_table_id = gaia::catalog::create_table("shape", fields);

    fields.emplace_back(std::make_unique<gaia::catalog::ddl::ref_field_def_t>("color", "", "color"));
    fields.emplace_back(std::make_unique<gaia::catalog::ddl::ref_field_def_t>("shape", "", "shape"));
    auto object_table_id = gaia::catalog::create_table("object", fields);

    begin_transaction();
    std::set<gaia_id_t> tables_with_relationship_to_object;
    for (relationship_view_t relationship : catalog_core_t::list_relationship_to(object_table_id))
    {
        tables_with_relationship_to_object.insert(relationship.parent_table_id());
    }
    ASSERT_EQ(tables_with_relationship_to_object, std::set({color_table_id, shape_table_id}));
    commit_transaction();
}
