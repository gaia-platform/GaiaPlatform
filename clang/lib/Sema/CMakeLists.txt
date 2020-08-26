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
  ${GAIA_BUILD}/production/catalog/catalog_manager/inc
  ${GAIA_BUILD}/production/inc/internal/catalog
  ${GAIA_BUILD}/production/inc/public/common
  ${GAIA_BUILD}/production/inc/public/direct_access
  ${GAIA_BUILD}/production/inc/internal/common
  ${GAIA_BUILD}/production/inc/internal/db
  ${GAIA_BUILD}/production/inc/public/db
  ${GAIA_BUILD}/production/inc/public/catalog
  ${GAIA_BUILD}/third_party/production/flatbuffers/include
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
  gaia_se_client
  gaia_direct
  gaia_types
  gaia_common
  gaia_catalog
  flatbuffers
  explain
  )
 target_include_directories(clangSema PRIVATE ${GAIA_CATALOG_INCLUDES})
 
add_library(flatbuffers STATIC IMPORTED GLOBAL)
set_target_properties(flatbuffers PROPERTIES IMPORTED_LOCATION ${GAIA_BUILD}/production/build/flatbuffers/libflatbuffers.a)

add_library(gaia_catalog STATIC IMPORTED GLOBAL)
set_target_properties(gaia_catalog PROPERTIES IMPORTED_LOCATION ${GAIA_BUILD}/production/build/catalog/catalog_manager/libgaia_catalog.a)

add_library(gaia_common STATIC IMPORTED GLOBAL)
set_target_properties(gaia_common PROPERTIES IMPORTED_LOCATION ${GAIA_BUILD}/production/build/common/libgaia_common.a)

add_library(gaia_se_client STATIC IMPORTED GLOBAL)
set_target_properties(gaia_se_client PROPERTIES IMPORTED_LOCATION ${GAIA_BUILD}/production/build/db/storage_engine/libgaia_se_client.a)

add_library(gaia_direct STATIC IMPORTED GLOBAL)
set_target_properties(gaia_direct PROPERTIES IMPORTED_LOCATION ${GAIA_BUILD}/production/build/direct_access/libgaia_direct.a)

add_library(gaia_types STATIC IMPORTED GLOBAL)
set_target_properties(gaia_types PROPERTIES IMPORTED_LOCATION ${GAIA_BUILD}/production/build/db/types/libgaia_types.a)
