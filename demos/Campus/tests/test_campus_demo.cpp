/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"
#include "../inc/campus_demo.hpp"
#include "../inc/message.hpp"

namespace tests{

// CampusDemo test fixture
class CampusDemoTest : public ::testing::Test {
 protected:

  CampusDemoTest() {
  }

  ~CampusDemoTest() override {
  }

  // called immidiately after constructor
  void SetUp() override {
  }

  // called immidiately before destructor
  void TearDown() override {
  }
};

// yada ...
TEST_F(CampusDemoTest, DemoTestTest) {
   CampusDemo::Campus cd;
   EXPECT_EQ(cd.DemoTest(), 0);
}

// yada ...
TEST_F(CampusDemoTest, MessageHeaderTest) {
   message::MessageHeader mh;
   EXPECT_EQ(mh.DemoTest(), 0);
}

// yada ...
TEST_F(CampusDemoTest, MessageTest) {
   message::Message<int> msg;
   EXPECT_EQ(msg.DemoTest(), 0);
}

}  // namespace tests

//int main(int argc, char **argv) {
//  ::testing::InitGoogleTest(&argc, argv);
//  return RUN_ALL_TESTS();
//}
