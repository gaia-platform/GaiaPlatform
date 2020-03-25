#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.10)
enable_testing()

# Set the project name.
project(direct_access)

set(GAIA_TEST_COMMON_INCLUDES 
    ${GAIA_INC}/public/common
    ${GAIA_INC}/internal/common
    ${GAIA_INC}/public/rules
    ${GAIA_RULES_INC}
    ${PROJECT_SOURCE_DIR}/inc
    ${FLATBUFFERS_INC}
)


set_property(GLOBAL PROPERTY FBS_GENERATED_OUTPUTS)

function(get_generated_output generated_files)
  get_property(tmp GLOBAL PROPERTY FBS_GENERATED_OUTPUTS)
  set(${generated_files} ${tmp} PARENT_SCOPE)
endfunction(get_generated_output)



function(register_generated_output file_name)
  get_property(tmp GLOBAL PROPERTY FBS_GENERATED_OUTPUTS)
  list(APPEND tmp ${file_name})
  set_property(GLOBAL PROPERTY FBS_GENERATED_OUTPUTS ${tmp})
endfunction(register_generated_output)

function(compile_flatbuffers_schema_to_cpp_opt SRC_FBS OPT)
  if(FLATBUFFERS_BUILD_LEGACY)
    set(OPT ${OPT};--cpp-std c++0x)
  else()
    # --cpp-std is defined by flatc default settings.
  endif()
  message(STATUS "`${SRC_FBS}`: add generation of C++ code with '${OPT}'")
  get_filename_component(SRC_FBS_DIR ${SRC_FBS} PATH)
  string(REGEX REPLACE "\\.fbs$" "_generated.h" GEN_HEADER ${SRC_FBS})
  add_custom_command(
    OUTPUT ${GEN_HEADER}
    COMMAND "${GAIA_REPO}/production/build/flatbuffers/flatc"
            --cpp --gen-mutable --gen-object-api --reflect-names
            --cpp-ptr-type flatbuffers::unique_ptr # Used to test with C++98 STLs
            ${OPT}
            -I "${CMAKE_CURRENT_SOURCE_DIR}"
            -o "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/${SRC_FBS}"
    DEPENDS flatc
    COMMENT "Run generation: '${GEN_HEADER}'")
    register_generated_output(${GEN_HEADER})
endfunction()

function(compile_flatbuffers_schema_to_cpp SRC_FBS)
  compile_flatbuffers_schema_to_cpp_opt(${SRC_FBS} "--no-includes;--gen-compare")
endfunction()



message(STATUS "GAIA_TEST_COMMON_INCLUDES=${GAIA_TEST_COMMON_INCLUDES}")
compile_flatbuffers_schema_to_cpp(addr_book.fbs "")

get_generated_output(fbs_generated)
if(fbs_generated)
  # message(STATUS "Add generated_code target with files:${fbs_generated}")
  add_custom_target(test_generated_code
    DEPENDS ${fbs_generated}
    COMMENT "All generated files were updated.")
endif()


# unit tests
add_executable(test_direct_access test_direct_access.cpp)
add_dependencies(test_direct_access test_generated_code)
set_target_properties(test_direct_access PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")

target_include_directories(test_direct_access PRIVATE 
    ${GAIA_TEST_COMMON_INCLUDES}
    ${GAIA_REPO}/production/db/storage_engine/mock
    ${GOOGLE_TEST_INC}
)

target_link_libraries(test_direct_access PRIVATE 
    gtest_main
    rt
)

gtest_discover_tests(test_direct_access)
