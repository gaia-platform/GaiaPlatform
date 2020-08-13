cmake_minimum_required(VERSION 3.11)

project(ASTCodeGenerator VERSION 0.0.0)

include(${CMAKE_BINARY_DIR}/../../../../production/cmake/gaia.cmake)

set(CMAKE_CXX_FLAGS
      "-std=c++0x -Wall -Wextra -ggdb -Wno-unused-parameter -DLLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING=1")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ggdb -pthread")


include_directories(
  ${CMAKE_BINARY_DIR}/../../../../../../TranslationEngineLLVM/TranslationEngineLLVM/clang/include
  ${CMAKE_BINARY_DIR}/../../../../../../TranslationEngineLLVM/TranslationEngineLLVM/build/tools/clang/include
  ${CMAKE_BINARY_DIR}/../../../../production/inc/internal/catalog
  ${CMAKE_BINARY_DIR}/../../../../production/catalog/catalog_manager/inc
  ${CMAKE_BINARY_DIR}/../../../../production/inc/public/common
  ${CMAKE_BINARY_DIR}/../../../../production/inc/public/catalog
  ${CMAKE_BINARY_DIR}/../../../../production/inc/public/direct_access
  ${CMAKE_BINARY_DIR}/../../../../production/inc/internal/common
  ${CMAKE_BINARY_DIR}/../../../../production/inc/internal/db
  ${CMAKE_BINARY_DIR}/../../../../production/inc/public/db
  ${CMAKE_BINARY_DIR}/../../../..//third_party/production/flatbuffers/include
  ${CMAKE_BINARY_DIR}/../../../..//production/db/storage_engine/mock
)
set(ASTGENERATOR_DEMO_SRCS
  ../demo.cpp
  )

link_directories(
  ${CMAKE_BINARY_DIR}/../../../../../../TranslationEngineLLVM/TranslationEngineLLVM/build/lib
)


set(GAIA_PROD_BUILD ${CMAKE_BINARY_DIR}/../../../../production/build)
add_executable(ASTCodeGenerator ${ASTGENERATOR_DEMO_SRCS})

target_link_libraries(ASTCodeGenerator
  PRIVATE
  rt
  z
  ncurses
  clangTooling
  clang
  clangDriver
  clangFrontend
  clangSerialization
  clangParse
  clangSema 
  clangEdit
  clangLex
  clangAST
  clangAnalysis
  clangBasic
  clangASTMatchers
  clangRewrite
  LLVMLTO 
  LLVMPasses
  LLVMObjCARCOpts
  LLVMMIRParser
  LLVMSymbolize
  LLVMDebugInfoPDB
  LLVMDebugInfoDWARF
  LLVMCoverage
  LLVMMCA
  LLVMTableGen
  LLVMDlltoolDriver
  LLVMXRay
  LLVMOrcJIT
  LLVMTestingSupport
  LLVMXCoreDisassembler
  LLVMXCoreCodeGen
  LLVMXCoreDesc
  LLVMXCoreInfo
  LLVMXCoreAsmPrinter
  LLVMWebAssemblyDisassembler
  LLVMWebAssemblyCodeGen
  LLVMWebAssemblyDesc
  LLVMWebAssemblyAsmPrinter
  LLVMWebAssemblyAsmParser
  LLVMWebAssemblyInfo
  LLVMSystemZDisassembler
  LLVMSystemZCodeGen 
  LLVMSystemZAsmParser
  LLVMSystemZDesc
  LLVMSystemZInfo
  LLVMSystemZAsmPrinter
  LLVMSparcDisassembler
  LLVMSparcCodeGen
  LLVMSparcAsmParser
  LLVMSparcDesc 
  LLVMSparcInfo
  LLVMSparcAsmPrinter
  LLVMPowerPCDisassembler
  LLVMPowerPCCodeGen
  LLVMPowerPCAsmParser
  LLVMPowerPCDesc
  LLVMPowerPCInfo 
  LLVMPowerPCAsmPrinter 
  LLVMNVPTXCodeGen 
  LLVMNVPTXDesc
  LLVMNVPTXInfo
  LLVMNVPTXAsmPrinter
  LLVMMSP430Disassembler 
  LLVMMSP430CodeGen 
  LLVMMSP430AsmParser
  LLVMMSP430Desc 
  LLVMMSP430Info 
  LLVMMSP430AsmPrinter
  LLVMMipsDisassembler
  LLVMMipsCodeGen
  LLVMMipsAsmParser
  LLVMMipsDesc
  LLVMMipsInfo 
  LLVMMipsAsmPrinter
  LLVMLanaiDisassembler
  LLVMLanaiCodeGen
  LLVMLanaiAsmParser 
  LLVMLanaiDesc 
  LLVMLanaiAsmPrinter 
  LLVMLanaiInfo
  LLVMHexagonDisassembler 
  LLVMHexagonCodeGen 
  LLVMHexagonAsmParser 
  LLVMHexagonDesc 
  LLVMHexagonInfo 
  LLVMBPFDisassembler 
  LLVMBPFCodeGen 
  LLVMBPFAsmParser 
  LLVMBPFDesc 
  LLVMBPFInfo 
  LLVMBPFAsmPrinter
  LLVMARMDisassembler 
  LLVMARMCodeGen 
  LLVMARMAsmParser 
  LLVMARMDesc 
  LLVMARMInfo 
  LLVMARMAsmPrinter 
  LLVMARMUtils 
  LLVMAMDGPUDisassembler 
  LLVMAMDGPUCodeGen 
  LLVMAMDGPUAsmParser 
  LLVMAMDGPUDesc 
  LLVMAMDGPUInfo 
  LLVMAMDGPUAsmPrinter 
  LLVMAMDGPUUtils 
  LLVMAArch64Disassembler 
  LLVMAArch64CodeGen 
  LLVMAArch64AsmParser 
  LLVMAArch64Desc 
  LLVMAArch64Info 
  LLVMAArch64AsmPrinter 
  LLVMAArch64Utils 
  LLVMObjectYAML 
  LLVMLibDriver 
  LLVMOption 
  LLVMOptRemarks 
  LLVMWindowsManifest 
  LLVMTextAPI 
  LLVMFuzzMutate 
  LLVMX86Disassembler 
  LLVMX86AsmParser 
  LLVMX86CodeGen 
  LLVMGlobalISel 
  LLVMSelectionDAG 
  LLVMAsmPrinter 
  LLVMX86Desc 
  LLVMMCDisassembler 
  LLVMX86Info 
  LLVMX86AsmPrinter 
  LLVMX86Utils 
  LLVMMCJIT 
  LLVMLineEditor 
  LLVMInterpreter 
  LLVMExecutionEngine 
  LLVMRuntimeDyld 
  LLVMCodeGen 
  LLVMTarget 
  LLVMCoroutines 
  LLVMipo 
  LLVMInstrumentation 
  LLVMVectorize 
  LLVMScalarOpts 
  LLVMLinker 
  LLVMIRReader 
  LLVMAsmParser 
  LLVMInstCombine 
  LLVMBitWriter 
  LLVMAggressiveInstCombine 
  LLVMTransformUtils 
  LLVMAnalysis 
  LLVMProfileData 
  LLVMObject 
  LLVMMCParser 
  LLVMMC 
  LLVMDebugInfoCodeView 
  LLVMDebugInfoMSF 
  LLVMBitReader 
  LLVMCore 
  LLVMBinaryFormat 
  LLVMSupport
  LLVMDemangle
  gaia_semock
  gaia_direct
  explain
)

add_library(gaia_semock STATIC IMPORTED GLOBAL)
set_target_properties(gaia_semock PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/db/storage_engine/libgaia_semock.a)

add_library(gaia_direct STATIC IMPORTED GLOBAL)
set_target_properties(gaia_direct PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/direct_access/libgaia_direct.a)


