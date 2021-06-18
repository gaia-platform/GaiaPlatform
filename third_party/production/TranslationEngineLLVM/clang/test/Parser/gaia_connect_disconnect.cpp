// RUN: %clang_cc1 -fgaia-extensions -ast-dump %s | FileCheck -strict-whitespace %s

#include "barn_storage/gaia_barn_storage.h"

ruleset connect1
{
    OnInsert(farmer)
    {
      gaia::barn_storage::incubator_t i;
      farmer.name = "suppini";
      farmer.gaia_id();
      farmer.Connect();

      farmer.name;
      farmer.incubators;
//      farmer.incubators.connect(i);
    }
}

// CHECK:      RulesetDecl{{.*}} connect1
// CHECK:      VarDecl 0x{{[^ ]*}} <col:7, col:39> col:39 i 'gaia::barn_storage::incubator_t':'gaia::barn_storage::incubator_t' callinit

// %clang_cc1 -fgaia-extensions -ast-dump -isystem %S/../Modules/Inputs/System/usr/include -I/usr/include/ -I/usr/include/x86_64-linux-gnu/ -I/home/simone/repos/GaiaPlatform/third_party/production/TranslationEngineLLVM/libcxx/include -I/home/simone/repos/GaiaPlatform/third_party/production/flatbuffers/include -I/home/simone/repos/GaiaPlatform/production/inc/ -I/home/simone/repos/GaiaPlatform/production/cmake-build-gaiarelease-release-llvmtests/gaia_generated/barn_storage %s | FileCheck -strict-whitespace %s
