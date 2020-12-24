/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "../inc/campus_demo.hpp"
#include "../inc/message.hpp"
#include "gtest/gtest.h"

namespace tests
{

// campus_demo test fixture
class campus_demo_test : public ::testing::Test
{
protected:
    campus_demo_test() = default;
    ~campus_demo_test() override = default;

    // called immidiately after constructor
    void SetUp() override
    {
    }

    // called immidiately before destructor
    void TearDown() override
    {
    }
};

// yada ...
TEST_F(campus_demo_test, DemoTestTest)
{
    campus_demo::campus cd;
    EXPECT_EQ(cd.demo_test(), 0);
}

// yada ...
TEST_F(campus_demo_test, MessageHeaderTest)
{
    bus_messages::message_header mh;
    EXPECT_EQ(mh.demo_test(), 0);
}

// yada ...
TEST_F(campus_demo_test, MessageTest)
{
    bus_messages::message msg;
    EXPECT_EQ(msg.demo_test(), 0);
}

} // namespace tests

//int main(int argc, char **argv) {
//  ::testing::InitGoogleTest(&argc, argv);
//  return RUN_ALL_TESTS();
//}
