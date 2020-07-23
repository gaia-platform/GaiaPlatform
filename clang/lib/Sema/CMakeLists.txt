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
  ../../../../../ASTCodeGeneration/GaiaPlatform/production/build/catalog/catalog_manager/generated
  ../../../../../ASTCodeGeneration/GaiaPlatform/production/inc/public/common
  ../../../../../ASTCodeGeneration/GaiaPlatform/production/inc/public/direct_access
  ../../../../../ASTCodeGeneration/GaiaPlatform/third_party/production/flatbuffers/include
  ../../../../../ASTCodeGeneration/GaiaPlatform/production/db/storage_engine/mock
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
  gaia_semock
  gaia_direct
  )
 target_include_directories(clangSema PRIVATE ${GAIA_CATALOG_INCLUDES})
 add_library(gaia_semock STATIC IMPORTED GLOBAL)
set_target_properties(gaia_semock PROPERTIES IMPORTED_LOCATION /home/gregory/src/ASTCodeGeneration/GaiaPlatform/production/build/db/storage_engine/libgaia_semock.a)

add_library(gaia_direct STATIC IMPORTED GLOBAL)
set_target_properties(gaia_direct PROPERTIES IMPORTED_LOCATION /home/gregory/src/ASTCodeGeneration/GaiaPlatform/production/build/direct_access/libgaia_direct.a)

