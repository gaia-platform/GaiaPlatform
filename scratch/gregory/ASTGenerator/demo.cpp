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
        : fieldGetMatcherHandler(r), fieldSetMatcherHandler(r), ruleMatcherHandler(r),
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
    }
private:
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
    if (ASTGeneratorVerboseOption)
    {
        g_verbose = true;
    }
    // Create a new Clang Tool instance (a LibTooling environment).
    ClangTool tool(op.getCompilations(), op.getSourcePathList());
    tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-fgaia-extensions"));
    tool.run(newFrontendActionFactory<ASTGenerator_Action>().get());
    
}
