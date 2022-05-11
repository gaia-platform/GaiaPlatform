/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <string>

#include <gtest/gtest.h>

#include "gaia/common.hpp"
#include "gaia/direct_access/auto_transaction.hpp"

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_barn_storage.h"

using namespace gaia::barn_storage;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::direct_access;

class direct_access__incubator : public db_catalog_test_base_t
{
protected:
    direct_access__incubator()
        : db_catalog_test_base_t(std::string("incubator.ddl")){};

    gaia_id_t insert_incubator(const char* name, float min_temp, float max_temp)
    {
        incubator_writer w;
        w.name = name;
        w.min_temp = min_temp;
        w.max_temp = max_temp;
        return w.insert_row();
    }
};

// Make sure incubator demo schema works for read and write.
TEST_F(direct_access__incubator, schema_read_write_test)
{
    const char c_chicken[] = "Simone";
    const float c_chicken_min = 76.7f;
    const float c_chicken_max = 83.2f;
    const char c_sensor_a[] = "temp";
    gaia_id_t incubator_id = c_invalid_gaia_id;
    {
        auto_transaction_t txn;
        incubator_id = insert_incubator(c_chicken, c_chicken_min, c_chicken_max);
        ASSERT_TRUE(incubator_id != c_invalid_gaia_id);
        auto incubator = incubator_t::get(incubator_id);
        auto sensor = sensor_t::insert_row(c_sensor_a, 0, c_chicken_min);
        incubator.sensors().insert(sensor);
        txn.commit();
    }

    {
        auto_transaction_t txn;
        auto incubator = incubator_t::get(incubator_id);
        ASSERT_STREQ(incubator.name(), c_chicken);
        ASSERT_FLOAT_EQ(incubator.max_temp(), c_chicken_max);
        ASSERT_FLOAT_EQ(incubator.min_temp(), c_chicken_min);
        auto sensor_list = incubator.sensors();
        ASSERT_EQ(1, std::distance(sensor_list.begin(), sensor_list.end()));
        ASSERT_STREQ(sensor_list.begin()->name(), c_sensor_a);
        ASSERT_FLOAT_EQ(sensor_list.begin()->value(), c_chicken_min);
    }
}
