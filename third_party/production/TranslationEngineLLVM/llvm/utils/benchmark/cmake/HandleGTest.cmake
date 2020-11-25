
include(split_list)

if (BENCHMARK_ENABLE_GTEST_TESTS)
  if (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/googletest)
    set(GTEST_ROOT "${CMAKE_SOURCE_DIR}/googletest")
    set(INSTALL_GTEST OFF CACHE INTERNAL "")
    set(INSTALL_GMOCK OFF CACHE INTERNAL "")
    add_subdirectory(${CMAKE_SOURCE_DIR}/googletest)
    set(GTEST_BOTH_LIBRARIES llvm_gtest llvm_gtest_main)
    foreach(HEADER test mock)
      # CMake 2.8 and older don't respect INTERFACE_INCLUDE_DIRECTORIES, so we
      # have to add the paths ourselves.
      set(HFILE g${HEADER}/g${HEADER}.h)
      set(HPATH ${GTEST_ROOT}/google${HEADER}/include)
      find_path(HEADER_PATH_${HEADER} ${HFILE}
          NO_DEFAULT_PATHS
          HINTS ${HPATH}
      )
      if (NOT HEADER_PATH_${HEADER})
        message(FATAL_ERROR "Failed to find header ${HFILE} in ${HPATH}")
      endif()
      list(APPEND GTEST_INCLUDE_DIRS ${HEADER_PATH_${HEADER}})
    endforeach()
  endif()
endif()
