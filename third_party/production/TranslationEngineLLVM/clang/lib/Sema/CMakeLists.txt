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
  ${GAIA_SOURCE}/production/catalog/inc
  ${GAIA_SOURCE}/production/inc/gaia_internal/catalog
  ${GAIA_SOURCE}/production/inc/
  ${GAIA_SOURCE}/production/inc/gaia_internal/common
  ${GAIA_SOURCE}/production/inc/gaia_internal/db
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
  )

# We should not need this right now because we are not pulling any headers that
# include the spdlog but set it here anyway in case this changes in upstream
# dependencies.
set_target_properties(clangSema PROPERTIES COMPILE_FLAGS "-DSPDLOG_FMT_EXTERNAL")

target_include_directories(clangSema PRIVATE ${GAIA_CATALOG_INCLUDES})
