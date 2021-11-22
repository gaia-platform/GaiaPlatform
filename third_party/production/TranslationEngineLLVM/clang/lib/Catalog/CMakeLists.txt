add_clang_library(clangCatalog
  GaiaCatalog.cpp
  LINK_LIBS
    gaia_system
    clangFrontend
)

# For some unknown reason the clangCatalog target is not able to transitively use the
# headers that are defined as part of the flatuffers library. For this reason we have added
# an explicit dependency to the flatbuffers library (to wait for the headers to be downloaded)
# and explicitly added the flatbuffers headers in the included directories.
add_dependencies(clangCatalog flatbuffers)
target_include_directories(clangCatalog PRIVATE
  ${GAIA_INC}
  ${FLATBUFFERS_INC}
)
