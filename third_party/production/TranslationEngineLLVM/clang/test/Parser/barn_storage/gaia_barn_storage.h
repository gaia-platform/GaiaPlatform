/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

// TODO I was not able to import the EDC types in these tests. There is always some import missing.
//  I created this file to "mock" the EDC types.
//  These are some of the includes I had to add to the clang path to resolve some of the import
//  problem. It is not enough even though it could be a start.
//  -  %S/../Modules/Inputs/System/usr/include
//  - -I/usr/include/
//  - -I/usr/include/x86_64-linux-gnu/
//  - -I/home/simone/repos/GaiaPlatform/third_party/production/TranslationEngineLLVM/libcxx/include
//  - -I/home/simone/repos/GaiaPlatform/third_party/production/flatbuffers/include
//  - -I/home/simone/repos/GaiaPlatform/production/inc/
//  - -I/home/simone/repos/GaiaPlatform/production/cmake-build-gaiarelease-release-llvmtests/gaia_generated/barn_storage
//   Note: many of the import problems comes from the "flatbuffers" import... If only we could get rid of them... (GAIAPLAT-681)

namespace gaia {
namespace barn_storage {

class incubator_t {
};

} // namespace barn_storage
} // namespace gaia
