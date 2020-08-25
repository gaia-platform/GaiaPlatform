/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"
#include "gaia_barn_storage.h"
#include "rules.hpp"
#include "gaia_catalog_internal.hpp"

using namespace std;
using namespace gaia::rules;
using namespace gaia::common;

extern int rule_called;
extern int insert_called;
extern int update_called;

gaia_id_t insert_incubator(const char * name, float min_temp, float max_temp) 
{
    gaia::barn_storage::incubator_writer w;
    w.name = name;
    w.min_temp = min_temp;
    w.max_temp = max_temp;
    return w.insert_row();
}

void init_storage() {
    gaia::db::begin_transaction();

    auto incubator = gaia::barn_storage::incubator_t::get(insert_incubator("TestIncubator", 99.0, 102.0));
    incubator.i__sensor_list().insert(gaia::barn_storage::sensor_t::insert_row(incubator.gaia_id(), "TestSensor1", 0, 0));
    incubator.i__actuator_list().insert(gaia::barn_storage::actuator_t::insert_row(incubator.gaia_id(), "TestActuator", 0, 0.0));

    gaia::db::commit_transaction();
}

TEST(translation_engine, subscribe_invalid_ruleset)
{ 
    EXPECT_THROW(subscribe_ruleset("bogus"), ruleset_not_found);
    EXPECT_THROW(unsubscribe_ruleset("bogus"), ruleset_not_found);
}

TEST(translation_engine, subscribe_valid_ruleset)
{
    
    gaia::db::begin_session();
    gaia::catalog::load_catalog("barn_storage.ddl");
    gaia::rules::initialize_rules_engine();
    
    init_storage();
    sleep(1);
    EXPECT_EQ(rule_called,1);
    EXPECT_EQ(insert_called,1);
    EXPECT_EQ(update_called,0);
    gaia::db::begin_transaction();
    
    for (auto i : gaia::barn_storage::incubator_t::list()) 
    {
        EXPECT_EQ(i.max_temp(),4);
    }

    for (auto s : gaia::barn_storage::sensor_t::list()) 
    {
        EXPECT_EQ(s.value(),0);
    }

    for (auto a : gaia::barn_storage::actuator_t::list()) 
    {
        EXPECT_EQ(a.value(),0);
    }
    gaia::db::commit_transaction();


    gaia::db::begin_transaction();
    
    for (auto s : gaia::barn_storage::sensor_t::list()) 
    {
        auto w = s.writer();
        w.value = 6;
        w.update_row();
    }
    gaia::db::commit_transaction();
    sleep(1);
    EXPECT_EQ(rule_called,2);
    EXPECT_EQ(insert_called,1);
    EXPECT_EQ(update_called,1);

    gaia::db::begin_transaction();
    
    for (auto i : gaia::barn_storage::incubator_t::list()) 
    {
        EXPECT_EQ(i.max_temp(),10);
    }

    for (auto s : gaia::barn_storage::sensor_t::list()) 
    {
        EXPECT_EQ(s.value(),6);
    }

    for (auto a : gaia::barn_storage::actuator_t::list()) 
    {
        EXPECT_EQ(a.value(),1000);
    }
    gaia::db::commit_transaction();
    gaia::db::end_session();
}