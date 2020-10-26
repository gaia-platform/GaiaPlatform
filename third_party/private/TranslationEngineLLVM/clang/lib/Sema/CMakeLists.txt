set(LLVM_LINK_COMPONENTS
  Support
  )

if (MSVC)
  set_source_files_properties(SemaDeclAttr.cpp PROPERTIES COMPILE_FLAGS /bigobj)
  set_source_files_properties(SemaExpr.cpp PROPERTIES COMPILE_FLAGS /bigobj)
  set_source_files_properties(SemaExprCXX.cpp PROPERTIES COMPILE_FLAGS /bigobj)
  set_source_files_properties(SemaTemplate.cpp PROPERTIES COMPILE_FLAGS /bigobj)
endif()

set(GAIA_CATALOG_INCLUDES
  ${GAIA_SOURCE}/production/catalog/catalog_manager/inc
  ${GAIA_SOURCE}/production/inc/internal/catalog
  ${GAIA_SOURCE}/production/inc/public/common
  ${GAIA_SOURCE}/production/inc/public/direct_access
  ${GAIA_SOURCE}/production/inc/internal/common
  ${GAIA_SOURCE}/production/inc/internal/db
  ${GAIA_SOURCE}/production/inc/public/db
  ${GAIA_SOURCE}/production/inc/public/catalog
  ${GAIA_SOURCE}/third_party/production/flatbuffers/include
)

add_clang_library(clangSema
  AnalysisBasedWarnings.cpp
  CodeCompleteConsumer.cpp
  DeclSpec.cpp
  DelayedDiagnostic.cpp
  IdentifierResolver.cpp
  JumpDiagnostics.cpp
  MultiplexExternalSemaSource.cpp
  ParsedAttr.cpp
  Scope.cpp
  ScopeInfo.cpp
  Sema.cpp
  SemaAccess.cpp
  SemaAttr.cpp
  SemaCXXScopeSpec.cpp
  SemaCast.cpp
  SemaChecking.cpp
  SemaCodeComplete.cpp
  SemaConsumer.cpp
  SemaCoroutine.cpp
  SemaCUDA.cpp
  SemaDecl.cpp
  SemaDeclAttr.cpp
  SemaDeclCXX.cpp
  SemaDeclObjC.cpp
  SemaExceptionSpec.cpp
  SemaExpr.cpp
  SemaExprCXX.cpp
  SemaExprMember.cpp
  SemaExprObjC.cpp
  SemaFixItUtils.cpp
  SemaInit.cpp
  SemaLambda.cpp
  SemaLookup.cpp
  SemaObjCProperty.cpp
  SemaOpenMP.cpp
  SemaOverload.cpp
  SemaPseudoObject.cpp
  SemaStmt.cpp
  SemaStmtAsm.cpp
  SemaStmtAttr.cpp
  SemaTemplate.cpp
  SemaTemplateDeduction.cpp
  SemaTemplateInstantiate.cpp
  SemaTemplateInstantiateDecl.cpp
  SemaTemplateVariadic.cpp
  SemaType.cpp
  TypeLocBuilder.cpp
  SemaGaia.cpp

  LINK_LIBS
  clangAST
  clangAnalysis
  clangBasic
  clangEdit
  clangLex
  gaia_se_client_llvm
  gaia_payload_types_llvm
  gaia_common_llvm
  gaia_catalog_llvm
  gaia_direct_llvm
  flatbuffers_llvm
  explain
  )


target_include_directories(clangSema PRIVATE ${GAIA_CATALOG_INCLUDES})
add_dependencies(clangSema flatbuffers gaia_catalog gaia_common gaia_se_client gaia_direct gaia_payload_types)

add_library(gaia_payload_types_llvm STATIC IMPORTED GLOBAL)
set_target_properties(gaia_payload_types_llvm PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/db/payload_types/libgaia_payload_types.a)

add_library(flatbuffers_llvm STATIC IMPORTED GLOBAL)
set_target_properties(flatbuffers_llvm PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/flatbuffers/libflatbuffers.a)

add_library(gaia_catalog_llvm STATIC IMPORTED GLOBAL)
set_target_properties(gaia_catalog_llvm PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/catalog/catalog_manager/libgaia_catalog.a)

add_library(gaia_common_llvm STATIC IMPORTED GLOBAL)
set_target_properties(gaia_common_llvm PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/common/libgaia_common.a)

add_library(gaia_se_client_llvm STATIC IMPORTED GLOBAL)
set_target_properties(gaia_se_client_llvm PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/db/storage_engine/libgaia_se_client.a)

add_library(gaia_direct_llvm STATIC IMPORTED GLOBAL)
set_target_properties(gaia_direct_llvm PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/direct_access/libgaia_direct.a)

target_include_directories(clangSema PRIVATE ${GAIA_CATALOG_INCLUDES})
