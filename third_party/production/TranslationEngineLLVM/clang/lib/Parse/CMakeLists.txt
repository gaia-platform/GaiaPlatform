set(LLVM_LINK_COMPONENTS
  MC
  MCParser
  Support
  )

add_clang_library(clangParse
  ParseAST.cpp
  ParseCXXInlineMethods.cpp
  ParseDecl.cpp
  ParseDeclCXX.cpp
  ParseExpr.cpp
  ParseExprCXX.cpp
  ParseInit.cpp
  ParseObjc.cpp
  ParseOpenMP.cpp
  ParsePragma.cpp
  ParseStmt.cpp
  ParseStmtAsm.cpp
  ParseTemplate.cpp
  ParseTentative.cpp
  Parser.cpp
  ParseGaia.cpp

  LINK_LIBS
  clangAST
  clangBasic
  clangLex
  clangSema
  gaia_system
  )
target_include_directories(clangParse PRIVATE ${GAIA_INC})
