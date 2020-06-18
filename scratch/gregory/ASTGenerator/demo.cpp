/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_set>

#include "clang/Driver/Options.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;
using namespace clang::ast_matchers;

cl::OptionCategory ASTGeneratorCategory("Use ASTGenerator options");
cl::opt<string> ASTGeneratorOutputOption("output", cl::init(""), 
    cl::desc("output file name"), cl::cat(ASTGeneratorCategory));
cl::opt<bool> ASTGeneratorVerboseOption("v",
    cl::desc("print parse tokens"), cl::cat(ASTGeneratorCategory));

std::string curRuleset;
bool g_verbose = false;

void log(const char* message, const char* value)
{
    if (g_verbose)
    {
        printf(message, value);
    }
}

void log(const char* message, const std::string& value)
{
    log(message, value.c_str());
}

// Trim from start.
string &ltrim(string &s) 
{
    s.erase(s.begin(), 
        find_if(s.begin(), s.end(), not1(ptr_fun<int, int>(isspace))));
    return s;
}

// Trim from end.
string &rtrim(string &s) 
{
    s.erase(
        find_if(s.rbegin(), s.rend(), not1(ptr_fun<int, int>(isspace))).base(), s.end());
    return s;
}

// Trim from both ends.
string &trim(string &s) 
{
    return ltrim(rtrim(s));
}

string removeSemicolon(string s)
{
    if (!s.empty() && *s.rbegin() == ';')
    {
        return s.substr(0, s.size()-1);
    }
    else
    {
        return s;
    }
    
}

void split(const string &text, vector<string>& tokens, char separator) 
{
    size_t start = 0, end = 0;
    string token;
    
    while ((end = text.find(separator, start)) != string::npos) 
    {
        token = text.substr(start, end - start);
        tokens.push_back(trim(token));
        log("token: %s\n", tokens.back());
        start = end + 1;
    }
    token  = text.substr(start);
    tokens.push_back(trim(token));
    log("token: %s\n", tokens.back());
}



class ASTGenerator_Visitor
  : public RecursiveASTVisitor<ASTGenerator_Visitor> 
{
public:

    explicit ASTGenerator_Visitor(ASTContext *context)
    {
        m_context = context;
    }

    virtual bool VisitRulesetDecl(clang::RulesetDecl* d) 
    {
        log("ruleset %s\n", d->getName().str());

        for (Attr * attr : d->getAttrs())
        {
            switch (attr->getKind())
            {
                case attr::RulesetTable:
                    log("Attribute %s\n","Table");
                    {
                        RulesetTableAttr * tAttr= dyn_cast<RulesetTableAttr>(attr);
                        for (const IdentifierInfo * id : tAttr->tables())
                        {
                            log (" Param %s \n", id->getName().str());
                        }
                    }
                    break;
                case attr::SerialStream:
                    log("Attribute %s\n","SerialStream");
                    {
                        SerialStreamAttr *ssAttr = dyn_cast<SerialStreamAttr>(attr);
                        log (" Param %s\n", ssAttr->getStream()->getName().str());
                    }
                    break;
                default:
                    log ("Unknown attribute %s\n"," ");
            }
            
        }

        return true;
    }

    std::string ProcessStatement(const Stmt *stmt)
    {
        std::string retVal;
        if (stmt == nullptr)
        {
            return retVal;
        }

        switch (stmt->getStmtClass())
        {
            default:
                log("Unsupported Statement\n","");
                break;

            case Stmt::BinaryOperatorClass:
                retVal = ProcessBinaryOperator(dyn_cast<BinaryOperator>(stmt));
                break;

            case Stmt::DeclRefExprClass:
                retVal = ProcessDeclRefExpr(dyn_cast<DeclRefExpr>(stmt));
                break;

            case Stmt::ImplicitCastExprClass:
            {
                const ImplicitCastExpr *exp = dyn_cast<ImplicitCastExpr>(stmt);
                if (exp != nullptr)
                {
                    retVal = ProcessStatement(exp->getSubExpr());
                }
                break;
            }

            case Stmt::CompoundAssignOperatorClass:
              retVal = ProcessCompoundAssignmentOperator(dyn_cast<CompoundAssignOperator>(stmt));
              break;

            case Stmt::IntegerLiteralClass:
            {
                const IntegerLiteral * exp = dyn_cast<IntegerLiteral>(stmt);
                if (exp != nullptr)
                {
                    retVal = exp->getValue().toString(10, exp->getType()->isUnsignedIntegerType());
                }
                break;
            }

            case Stmt::FixedPointLiteralClass:
            {
                const FixedPointLiteral * exp = dyn_cast<FixedPointLiteral>(stmt);
                if (exp != nullptr)
                {
                    retVal = exp->getValueAsString(10);
                }
                break;
            }

            case Stmt::FloatingLiteralClass:
            {
                const FloatingLiteral * exp = dyn_cast<FloatingLiteral>(stmt);
                if (exp != nullptr)
                {
                    llvm::SmallVector<char, 100> buffer;
                    exp->getValue().toString(buffer);
                    retVal = std::string(buffer.data(), buffer.size());
                }
                break;
            }

            case Stmt::DeclStmtClass:
                retVal = ProcessDeclExpr(dyn_cast<DeclStmt>(stmt));
                break;

            case Stmt::IfStmtClass:
                retVal = ProcessIfStmt(dyn_cast<IfStmt>(stmt));
                break;

            case Stmt::CompoundStmtClass:
                retVal = ProcessCompoundStmt(dyn_cast<CompoundStmt>(stmt));
                break;
            
            case Stmt::ForStmtClass:
                retVal = ProcessForStmt(dyn_cast<ForStmt>(stmt));
                break;


        }

        return retVal;        

    }

    std::string ProcessForStmt(const ForStmt *stmt)
    {
        std::string retVal;
        if (stmt == nullptr)
        {
            return retVal;
        }
        retVal = "for(";
        if (stmt->getInit() != nullptr) 
        {
            retVal += removeSemicolon(ProcessStatement(stmt->getInit()));
        }
        retVal += +";";
        if (stmt->getCond() != nullptr)
        {
            retVal += removeSemicolon(ProcessStatement(stmt->getCond()));
        }
        retVal += ";";
        if (stmt->getInc() != nullptr)
        {
            retVal += removeSemicolon(ProcessStatement(stmt->getInc()));
        }
        retVal += ")\n{\n";
        retVal += ProcessStatement(stmt->getBody()) ;
        retVal += "\n}\n";
        return retVal;
    }

    std::string ProcessCompoundStmt(const CompoundStmt *stmt)
    {
        std::string retVal;
        if (stmt == nullptr)
        {
            return retVal;
        }

        for (const Stmt *childStmt : stmt->children())
        {
            retVal += ProcessStatement(childStmt) + "\n";
        }
        return retVal;
    }

    std::string ProcessIfStmt(const IfStmt *stmt)
    {
        std::string retVal;
        if (stmt == nullptr)
        {
            return retVal;
        }
        retVal = "if(" + removeSemicolon(ProcessStatement(stmt->getCond())) + ")\n{\n" + ProcessStatement(stmt->getThen()) + "}\n";
        if (stmt->getElse() != nullptr)
        {
            retVal += "else\n{\n" + ProcessStatement(stmt->getElse()) + "}\n";
        }
        return retVal;

    }

    std::string ProcessVarDecl(const VarDecl *exp)
    {
        std::string retVal;
        if (exp == nullptr)
        {
            return retVal;
        }
        QualType type = exp->getType();
        retVal = type.getAsString() + " " + exp->getNameAsString();
        const Expr *init = exp->getInit();
        if (init == nullptr)
        {
            return retVal + ";";
        }

        return retVal + " = " + ProcessStatement(init) + ";";
    }

    std::string ProcessDecl(const Decl *exp)
    {
        switch (exp->getKind())
        {
            default:
                return "";
            case Decl::Var:
            {
                const VarDecl *decl = dyn_cast<VarDecl>(exp);
                return ProcessVarDecl(decl);
            }
        }
    }


    std::string ProcessDeclExpr(const DeclStmt *exp)
    {
        std::string retVal;
        if (exp == nullptr)
        {
            return retVal;
        }
        for (Decl *decl : exp->decls())
        {
            retVal += ProcessDecl(decl);
        }

        return retVal;
    }

    std::string ProcessDeclRefExpr(const DeclRefExpr *exp)
    {
        if (exp == nullptr)
        {
            return "";
        }
        const ValueDecl *decl = exp->getDecl();
        if (decl->hasAttr<GaiaFieldAttr>() || decl->hasAttr<GaiaFieldValueAttr>())
        {
            return decl->getName().str() + ".get()";
        }
        else if (decl->hasAttr<GaiaLastOperationUPDATEAttr>())
        {
            return "UPDATE";
        }
        else if (decl->hasAttr<GaiaLastOperationINSERTAttr>())
        {
            return "INSERT";
        }
        else if (decl->hasAttr<GaiaLastOperationDELETEAttr>())
        {
            return "DELETE";
        }
        else if (decl->hasAttr<GaiaLastOperationAttr>())
        {
            return decl->getName().str() + ".GetLastOperation()";
        }

        return decl->getName().str();
    }

    std::string ProcessBinaryOpcode(BinaryOperator::Opcode opcode)
    {
        switch(opcode)
        {
            case BO_Mul:
                return "*";
            case BO_Div:
                return "/";
            case BO_Rem:
                return "%";
            case BO_Add:
                return "+";
            case BO_Sub:
                return "-";
            case BO_Shl:
                return "<<";
            case BO_Shr:
                return ">>";
            case BO_LT:
                return "<";
            case BO_GT:
                return ">";
            case BO_LE:
                return "<=";
            case BO_GE:
                return ">=";
            case BO_EQ:
                return "==";
            case BO_NE:
                return "!=";
            case BO_And:
                return "&";
            case BO_Xor:
                return "^";
            case BO_Or:
                return "|";
            case BO_LAnd:
                return "&&";
            case BO_LOr:
                return "||";
            case BO_Assign:
                return  "=";
            case BO_MulAssign:
                return  "*=";
            case BO_DivAssign:
                return  "/=";
            case BO_RemAssign:
                return  "%=";
            case BO_AddAssign:
                return  "+=";
            case BO_SubAssign:
                return  "-=";
            case BO_ShlAssign:
                return  "<<=";
            case BO_ShrAssign:
                return  ">>=";
            case BO_AndAssign:
                return  "&=";
            case BO_XorAssign:
                return  "^=";
            case BO_OrAssign:
                return  "|=";
            default:
                return "";
        }
    }

    std::string ConvertCompoundBinaryOpcode(BinaryOperator::Opcode opcode)
    {
        switch(opcode)
        {
            case BO_MulAssign:
                return  "*";
            case BO_DivAssign:
                return  "/";
            case BO_RemAssign:
                return  "%";
            case BO_AddAssign:
                return  "+";
            case BO_SubAssign:
                return  "-";
            case BO_ShlAssign:
                return  "<<";
            case BO_ShrAssign:
                return  ">>";
            case BO_AndAssign:
                return  "&";
            case BO_XorAssign:
                return  "^";
            case BO_OrAssign:
                return  "|";
            default:
                return "";
        }
    }

    std::string ProcessCompoundAssignmentOperator(const CompoundAssignOperator *op)
    {
        if (op == nullptr)
        {
            return "";
        }

        Expr *l = op->getLHS();
        Expr *r = op->getRHS();
        BinaryOperator::Opcode opcode = op->getOpcode();
        DeclRefExpr *leftDeclExpr = dyn_cast<DeclRefExpr>(l);
        if (leftDeclExpr != nullptr)
        {
            ValueDecl *decl = leftDeclExpr->getDecl();
            if (decl->hasAttr<GaiaFieldAttr>() || decl->hasAttr<GaiaFieldValueAttr>())
            {
                return decl->getName().str() + ".set(" + ProcessStatement(l) 
                    + ConvertCompoundBinaryOpcode(opcode) + ProcessStatement(r) + ");";
            }
        }
        return ProcessStatement(l) + ProcessBinaryOpcode(opcode) + ProcessStatement(r) + ";";
    }

    std::string ProcessBinaryOperator(const BinaryOperator *op)
    {
        if (op == nullptr)
        {
            return "";
        }

        Expr *l = op->getLHS();
        Expr *r = op->getRHS();
        BinaryOperator::Opcode opcode = op->getOpcode();

        DeclRefExpr *leftDeclExpr = dyn_cast<DeclRefExpr>(l);
        if (leftDeclExpr != nullptr)
        {
            ValueDecl *decl = leftDeclExpr->getDecl();
            if ((decl->hasAttr<GaiaFieldAttr>() || decl->hasAttr<GaiaFieldValueAttr>()) && opcode == BO_Assign)
            {
                return decl->getName().str() + ".set(" + removeSemicolon(ProcessStatement(r)) + ");";
            }
        }
        return ProcessStatement(l) + ProcessBinaryOpcode(opcode) + ProcessStatement(r) + ";";
    }


    // Parse comments for function declaration
    virtual bool VisitFunctionDecl(FunctionDecl *d) 
    {
        log("\nfunction %s", d->getName().str());
        
        if (!d->hasAttr<RuleAttr>())
        {
            return true;
        }
        log(" Rule\n{\n","");
        log ("%s\n", ProcessStatement(d->getBody()));
        log("}\n","");


        return true;
    }

private:
    ASTContext * m_context;
};

class FieldGetMatchHandler : public MatchFinder::MatchCallback
{
public:
    FieldGetMatchHandler(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        //ASTContext *context = Result.Context;
        const DeclRefExpr *exp = Result.Nodes.getNodeAs<DeclRefExpr>("fieldGet");
        
        if (exp)
        {
            const ValueDecl *decl = exp->getDecl();
            
            if (decl->hasAttr<GaiaFieldAttr>())
            {
                rewriter.ReplaceText(SourceRange(exp->getLocation(),exp->getEndLoc()), decl->getName().str() + ".get()");
            }
            else if (decl->hasAttr<GaiaFieldValueAttr>())
            {
                rewriter.ReplaceText(SourceRange(exp->getLocation().getLocWithOffset(-1),exp->getEndLoc()), decl->getName().str() + ".get()");
            }
        }
    }

private:
    Rewriter &rewriter;
};

class FieldSetMatchHandler : public MatchFinder::MatchCallback
{
public:
    FieldSetMatchHandler(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        const BinaryOperator *op = Result.Nodes.getNodeAs<BinaryOperator>("fieldSet");
        if (op != nullptr)
        {
            const Expr *opExp = op->getLHS();
            if (opExp != nullptr)
            {
                const DeclRefExpr *leftDeclExpr = dyn_cast<DeclRefExpr>(opExp);
        
                if (leftDeclExpr != nullptr)
                {
                    const ValueDecl *opDecl = leftDeclExpr->getDecl();
                    tok::TokenKind tokenKind;
                    std::string replacementText = "[&]() mutable {" + opDecl->getName().str() + ".set(";
                    switch(op->getOpcode())
                    {
                        case BO_Assign:
                        {
                            tokenKind = tok::equal;
                            break;
                        }
                        case BO_MulAssign:
                        {
                            tokenKind = tok::starequal;
                            break;
                        }
                        case BO_DivAssign:
                        {
                            tokenKind = tok::slashequal;
                            break;
                        }
                        case BO_RemAssign:
                        {
                            tokenKind = tok::percentequal;
                            break;
                        }
                        case BO_AddAssign:
                        {
                            tokenKind = tok::plusequal;
                            break;
                        }
                        case BO_SubAssign:
                        {
                            tokenKind = tok::minusequal;
                            break;
                        }
                        case BO_ShlAssign:
                        {
                            tokenKind = tok::lesslessequal;
                            break;
                        }
                        case BO_ShrAssign:
                        {
                            tokenKind = tok::greatergreaterequal;
                            break;
                        }
                        case BO_AndAssign:
                        {
                            tokenKind = tok::ampequal;
                            break;
                        }
                        case BO_XorAssign:
                        {
                            tokenKind = tok::caretequal;
                            break;
                        }
                        case BO_OrAssign:
                        {
                            tokenKind = tok::pipeequal;
                            break;
                        }
                        default:
                            return;
                    }

                    if (op->getOpcode() != BO_Assign)
                    {
                        replacementText += opDecl->getName().str() + ".get() " + ConvertCompoundBinaryOpcode(op->getOpcode()) + "(";
                    }
                    
                    SourceLocation setLocEnd = Lexer::findLocationAfterToken(leftDeclExpr->getLocation(),tokenKind,rewriter.getSourceMgr(),rewriter.getLangOpts(),true);
                    rewriter.ReplaceText(SourceRange(leftDeclExpr->getLocation(),setLocEnd.getLocWithOffset(-1)), replacementText);
                    rewriter.InsertTextAfterToken(op->getEndLoc(),")");
                    if (op->getOpcode() != BO_Assign)
                    {
                        rewriter.InsertTextAfterToken(op->getEndLoc(),"); return " + opDecl->getName().str() + ".get();}() ");
                    }
                    else
                    {
                        rewriter.InsertTextAfterToken(op->getEndLoc(),";return " + opDecl->getName().str() + ".get();}() ");
                    }
                    
                }
            }
        }
    }

private:
    std::string ConvertCompoundBinaryOpcode(BinaryOperator::Opcode opcode)
    {
        switch(opcode)
        {
            case BO_MulAssign:
                return  "*";
            case BO_DivAssign:
                return  "/";
            case BO_RemAssign:
                return  "%";
            case BO_AddAssign:
                return  "+";
            case BO_SubAssign:
                return  "-";
            case BO_ShlAssign:
                return  "<<";
            case BO_ShrAssign:
                return  ">>";
            case BO_AndAssign:
                return  "&";
            case BO_XorAssign:
                return  "^";
            case BO_OrAssign:
                return  "|";
            default:
                return "";
        }
    }

    Rewriter &rewriter;
};

class FieldUnaryOperatorMatchHandler : public MatchFinder::MatchCallback
{
public:
    FieldUnaryOperatorMatchHandler(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
       const UnaryOperator *op = Result.Nodes.getNodeAs<UnaryOperator>("fieldUnaryOp");
        if (op != nullptr)
        {
            const Expr *opExp = op->getSubExpr();
            if (opExp != nullptr)
            {
                const DeclRefExpr *declExpr = dyn_cast<DeclRefExpr>(opExp);
        
                if (declExpr != nullptr)
                {
                    const ValueDecl *opDecl = declExpr->getDecl();
                    std::string replaceStr;
                    
                    if (op->isPostfix())
                    {
                        if (op->isIncrementOp())
                        {
                            replaceStr = "[&]() mutable {auto t=" + opDecl->getName().str() + ".get();" + opDecl->getName().str() + ".set(" +  opDecl->getName().str() + ".get() + 1); return t;}()";
                        }
                        else
                        {
                            replaceStr = "[&]() mutable {auto t=" + opDecl->getName().str() + ".get();" + opDecl->getName().str() + ".set(" +  opDecl->getName().str() + ".get() - 1); return t;}()";
                        }
                    }
                    else
                    {
                        if (op->isIncrementOp())
                        {
                            replaceStr = "[&]() mutable {" + opDecl->getName().str() + ".set(" +  opDecl->getName().str() + ".get() + 1); return " + opDecl->getName().str() + ".get();}()";
                        }
                        else
                        {
                            replaceStr = "[&]() mutable {" + opDecl->getName().str() + ".set(" +  opDecl->getName().str() + ".get() - 1); return " + opDecl->getName().str() + ".get();}()";
                        }
                    }

                    rewriter.ReplaceText(SourceRange(op->getBeginLoc().getLocWithOffset(-1),op->getEndLoc().getLocWithOffset(1)), replaceStr);

                }
            }
        }
 
    }

private:
    

    Rewriter &rewriter;
};

class RuleMatchHandler : public MatchFinder::MatchCallback
{
public:
    RuleMatchHandler(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        const FunctionDecl * ruleDecl = Result.Nodes.getNodeAs<FunctionDecl>("ruleDecl");
        if (ruleDecl != nullptr)
        {
            rewriter.InsertText(ruleDecl->getLocation(),"void " + curRuleset + "_" + ruleDecl->getName().str() +"()\n");
        }
    }

private:
    Rewriter &rewriter;
};

class RulesetMatchHandler : public MatchFinder::MatchCallback
{
public:
    RulesetMatchHandler(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        const RulesetDecl * rulesetDecl = Result.Nodes.getNodeAs<RulesetDecl>("rulesetDecl");
        if (rulesetDecl != nullptr)
        {
            curRuleset = rulesetDecl->getName().str();           
            rewriter.ReplaceText(SourceRange(rulesetDecl->getBeginLoc(),rulesetDecl->decls_begin()->getBeginLoc().getLocWithOffset(-2)),
                "namespace " + curRuleset + "\n{\n");
        }
    }

private:
    Rewriter &rewriter;
};


class ASTGenerator_Consumer : public clang::ASTConsumer 
{
public:
    explicit ASTGenerator_Consumer(ASTContext *context, Rewriter &r)
        : /*visitor(context) ,*/ fieldGetMatcherHandler(r), fieldSetMatcherHandler(r), ruleMatcherHandler(r),
        rulesetMatcherHandler(r), fieldUnaryOperatorMatchHandler(r)
    {
        StatementMatcher fieldGetMatcher = 
            declRefExpr(to(varDecl(anyOf(hasAttr(attr::GaiaField),hasAttr(attr::GaiaFieldValue)),
            unless(hasAttr(attr::GaiaFieldLValue))))).bind("fieldGet");  
        StatementMatcher fieldSetMatcher = binaryOperator(allOf(
            isAssignmentOperator(),
            hasLHS(declRefExpr(to(varDecl(hasAttr(attr::GaiaFieldLValue))))))).bind("fieldSet");
        DeclarationMatcher ruleMatcher = functionDecl(hasAttr(attr::Rule)).bind("ruleDecl");
        DeclarationMatcher rulesetMatcher = rulesetDecl().bind("rulesetDecl");
        StatementMatcher fieldUnaryOperatorMatcher = unaryOperator(allOf(anyOf(
            hasOperatorName("++"), hasOperatorName("--")),
            hasUnaryOperand(declRefExpr(to(varDecl(hasAttr(attr::GaiaFieldLValue)))))
            )).bind("fieldUnaryOp");
        
        matcher.addMatcher(fieldSetMatcher, &fieldSetMatcherHandler);
        matcher.addMatcher(fieldGetMatcher, &fieldGetMatcherHandler);
        matcher.addMatcher(ruleMatcher, &ruleMatcherHandler);
        matcher.addMatcher(rulesetMatcher, &rulesetMatcherHandler);
        matcher.addMatcher(fieldUnaryOperatorMatcher, &fieldUnaryOperatorMatchHandler);
    }

    virtual void HandleTranslationUnit(clang::ASTContext &context) 
    {
        matcher.matchAST(context);
       // visitor.TraverseDecl(context.getTranslationUnitDecl());
    }
private:
    //ASTGenerator_Visitor visitor;
    MatchFinder matcher;
    FieldGetMatchHandler fieldGetMatcherHandler;
    FieldSetMatchHandler fieldSetMatcherHandler;
    RuleMatchHandler     ruleMatcherHandler;
    RulesetMatchHandler  rulesetMatcherHandler;
    FieldUnaryOperatorMatchHandler fieldUnaryOperatorMatchHandler;
};

class ASTGenerator_Action : public clang::ASTFrontendAction 
{
public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance &compiler, llvm::StringRef inFile) 
    {
        AstGeneratorRewriter.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());
        return std::unique_ptr<clang::ASTConsumer>(
            new ASTGenerator_Consumer(&compiler.getASTContext(), AstGeneratorRewriter));
    }
    void EndSourceFileAction()
    {
        AstGeneratorRewriter.getEditBuffer(AstGeneratorRewriter.getSourceMgr().getMainFileID())
            .write(llvm::outs());
    }
private: 
    Rewriter AstGeneratorRewriter;
};

bool SaveFile(const char *name, const stringstream& buf) 
{
    std::ofstream ofs(name, std::ofstream::out);
    if (!ofs.is_open())
    {
        return false;
    }
    ofs << buf.str();
    return !ofs.bad();
}


int main(int argc, const char **argv) 
{

    // Parse the command-line args passed to your code.
    CommonOptionsParser op(argc, argv, ASTGeneratorCategory);
    //if (ASTGeneratorVerboseOption)
    {
        g_verbose = true;
    }
    // Create a new Clang Tool instance (a LibTooling environment).
    ClangTool tool(op.getCompilations(), op.getSourcePathList());
    tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-fgaia-extensions"));
    tool.run(newFrontendActionFactory<ASTGenerator_Action>().get());
    
}
