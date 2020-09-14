/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include <unistd.h>
#include "gtest/gtest.h"
#include "gaia_barn_storage.h"
#include "rules.hpp"
#include "gaia_catalog_internal.hpp"
#include "db_test_base.hpp"

using namespace std;
using namespace gaia::rules;
using namespace gaia::common;

extern int rule_called;
extern int insert_called;
extern int update_called;

class translation_engine_test : public db_test_base_t
{
public:
    gaia_id_t insert_incubator(const char * name, float min_temp, float max_temp) 
    {
        gaia::barn_storage::incubator_writer w;
        w.name = name;
        w.min_temp = min_temp;
        w.max_temp = max_temp;
        return w.insert_row();
    }

    void init_storage() 
    {
        gaia::db::begin_transaction();
        auto incubator = gaia::barn_storage::incubator_t::get(insert_incubator("TestIncubator", 99.0, 102.0));
        incubator.sensor_list().insert(gaia::barn_storage::sensor_t::insert_row("TestSensor1", 0, 0.0));
        incubator.actuator_list().insert(gaia::barn_storage::actuator_t::insert_row("TestActuator1", 0, 0.0));
        gaia::db::commit_transaction();
    }

protected:
    static void SetUpTestSuite()
    {
        // NOTE: to run this test manually, you need to set the env variable INCUBATOR_DDL_FILE
        // to the location of barn_storage.ddl.  Currently this is under production/schemas/test/incubator.
        reset_server();
        const char *incubator_ddl_file = getenv("INCUBATOR_DDL_FILE");
        ASSERT_NE(incubator_ddl_file, nullptr);
        gaia::db::begin_session();
        gaia::catalog::load_catalog(incubator_ddl_file);
        gaia::rules::initialize_rules_engine();
    }

    static void TearDownTestSuite()
    {
        gaia::db::end_session();
    }

    // Ensure SetUp and TearDown don't do anything.  When we run the test
    // directly, we only want SetUpTestSuite and TearDownTestSuite 
    void SetUp() override {}

    void TearDown() override 
    {
        unsubscribe_rules();
    }
};


TEST_F(translation_engine_test, subscribe_invalid_ruleset)
{ 
    EXPECT_THROW(subscribe_ruleset("bogus"), ruleset_not_found);
    EXPECT_THROW(unsubscribe_ruleset("bogus"), ruleset_not_found);
}

TEST_F(translation_engine_test, subscribe_valid_ruleset)
{
    init_storage();
    while (rule_called == 0) {usleep(50000);}
    
    EXPECT_EQ(rule_called,1);
    EXPECT_EQ(insert_called,1);
    EXPECT_EQ(update_called,0);

    gaia::db::begin_transaction();

    for (auto i : gaia::barn_storage::incubator_t::list()) 
    {
        EXPECT_EQ(i.max_temp(),4);
    }

    for (auto a : gaia::barn_storage::actuator_t::list()) 
    {
        EXPECT_EQ(a.value(),0);
    }

    for (auto s : gaia::barn_storage::sensor_t::list()) 
    {
        EXPECT_EQ(s.value(),0);
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

    while (rule_called == 1) {usleep(50000);}

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

