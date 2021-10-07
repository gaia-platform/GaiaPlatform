set(GAIA_CATALOG_INCLUDES
  ${GAIA_INC}
  ${FLATBUFFERS_INC}
)

add_clang_library(clangCatalog
  GaiaCatalog.cpp
  LINK_LIBS
  gaia_system
  clangFrontend
  )

target_include_directories(clangCatalog PRIVATE ${GAIA_CATALOG_INCLUDES})
