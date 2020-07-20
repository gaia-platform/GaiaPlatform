/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include "gtest/gtest.h"
#include "../addr_book_gaia_generated.h"
#include "gaia_system.hpp"
#include "rules.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::rules;
using namespace gaia::common;
using namespace AddrBook;

static uint32_t rule_count = 0;

const gaia_type_t m_gaia_type = 0;
extern "C"
void initialize_rules() {

}

void rule1(const rule_context_t* context)
{
    rule_count++;
}

rule_binding_t m_rule1{"ruleset1_name", "rule1_name", rule1};

class gaia_system_test : public ::testing::Test {
public:
protected:
    void SetUp() override {
        gaia::system::initialize(true);
    }

    void TearDown() override {
        // Delete the shared memory segments.
        gaia_mem_base::reset();
    }
};

// Insert one row and check if trigger is invoked
TEST_F(gaia_system_test, get_field) {
    EXPECT_EQ(rule_count, 0);
    subscribe_rule(m_gaia_type, event_type_t::row_insert, empty_fields, m_rule1);

    begin_transaction();
    auto e = new Employee();
    e->set_name_first("msj");
    e->insert_row();
    Employee::get_first()->delete_row();
    commit_transaction();

    EXPECT_EQ(rule_count, 1);
    unsubscribe_rules();
}
