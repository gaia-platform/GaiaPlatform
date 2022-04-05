//
// Created by simone on 1/29/22.
//

#include <string>

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>

using namespace clang;
using namespace ast_matchers;
using namespace clang::tooling;
using namespace llvm;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory g_my_tool_category("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp g_common_help(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp g_more_help("\nMore help text...\n");

cl::opt<std::string> g_translation_engine_output_option("output", cl::desc("output file name"), cl::init(""), cl::cat(g_my_tool_category));

class IfStmtHandler : public MatchFinder::MatchCallback
{
public:
    IfStmtHandler(Rewriter& Rewrite)
        : m_rewrite(Rewrite)
    {
    }

    virtual void run(const MatchFinder::MatchResult& Result)
    {
        // The matched 'if' statement was bound to 'ifStmt'.
        if (const IfStmt* IfS = Result.Nodes.getNodeAs<clang::IfStmt>("ifStmt"))
        {
            const Stmt* Then = IfS->getThen();
            m_rewrite.InsertText(Then->getBeginLoc(), "// the 'if' part\n", true, true);

            if (const Stmt* Else = IfS->getElse())
            {
                m_rewrite.InsertText(Else->getEndLoc(), "// the 'else' part\n", true, true);
            }
        }
    }

private:
    Rewriter& m_rewrite;
};

class function_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit function_handler_t(Rewriter& rewrite)
        : m_rewrite(rewrite){};

    void run(const MatchFinder::MatchResult& result) override
    {
        llvm::outs() << "function_handler_t\n";
        const auto* function_decl = result.Nodes.getNodeAs<FunctionDecl>("funcDecl");

        if (!function_decl)
        {
            llvm::outs() << "What da fuck\n";
        }

        for (auto attr : function_decl->attrs())
        {
            llvm::outs() << "1 Found attr: " << attr->getSpelling() << "\n";
        }
    }

private:
    Rewriter& m_rewrite;
};

class gaia_rule_attr_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit gaia_rule_attr_handler_t(Rewriter& rewrite)
        : m_rewrite(rewrite){};

    void run(const MatchFinder::MatchResult& result) override
    {
        llvm::outs() << "gaia_rule_attr_handler_t\n";

        const auto* function_decl = result.Nodes.getNodeAs<FunctionDecl>("ruleDecl");

        if (!function_decl)
        {
            llvm::outs() << "What da fuck\n";
        }

        for (auto attr : function_decl->attrs())
        {
            llvm::outs() << "2 Found attr: " << attr->getSpelling() << "\n";
        }
    }

private:
    Rewriter& m_rewrite;
};

class translation_engine_consumer_t : public clang::ASTConsumer
{
public:
    explicit translation_engine_consumer_t(ASTContext* ast, Rewriter& r)
        : m_rewriter(r), m_handler_for_if(r), m_function_handler(r), m_gaia_rule_attr_handler(r)
    {
        m_matcher.addMatcher(functionDecl().bind("funcDecl"), &m_function_handler);
        m_matcher.addMatcher(ifStmt().bind("ifStmt"), &m_handler_for_if);
        m_matcher.addMatcher(functionDecl(hasAttr(attr::GaiaRule)).bind("ruleDecl"), &m_gaia_rule_attr_handler);
    }

    void HandleTranslationUnit(clang::ASTContext& context) override
    {
        m_matcher.matchAST(context);
    }

private:
    MatchFinder m_matcher;
    Rewriter m_rewriter;

    IfStmtHandler m_handler_for_if;
    function_handler_t m_function_handler;
    gaia_rule_attr_handler_t m_gaia_rule_attr_handler;
};

class translation_engine_action_t : public clang::ASTFrontendAction
{
public:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance& compiler, llvm::StringRef) override
    {
        m_rewriter.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());

        return std::unique_ptr<clang::ASTConsumer>(
            new translation_engine_consumer_t(&compiler.getASTContext(), m_rewriter));
    }

    void EndSourceFileAction() override
    {
        std::error_code error_code;
        llvm::raw_fd_ostream output_file(g_translation_engine_output_option, error_code, llvm::sys::fs::F_None);

        m_rewriter.getEditBuffer(m_rewriter.getSourceMgr().getMainFileID())
            .write(llvm::outs());
        m_rewriter.getEditBuffer(m_rewriter.getSourceMgr().getMainFileID())
            .write(output_file);
    }

private:
    Rewriter m_rewriter;
};

int main(int argc, const char** argv)
{
    auto expected_parser = CommonOptionsParser::create(argc, argv, g_my_tool_category, llvm::cl::NumOccurrencesFlag::Optional);
    if (!expected_parser)
    {
        // Fail gracefully for unsupported options.
        llvm::errs() << expected_parser.takeError();
        return 1;
    }
    CommonOptionsParser& options_parser = expected_parser.get();

    std::string error_msg;
    std::unique_ptr<CompilationDatabase> compilation_database
        = FixedCompilationDatabase::loadFromCommandLine(argc, argv, error_msg);

    if (!compilation_database)
    {
        compilation_database = llvm::make_unique<clang::tooling::FixedCompilationDatabase>(
            ".", std::vector<std::string>());
    }

    ClangTool tool(*compilation_database, options_parser.getSourcePathList());

    int result = tool.run(newFrontendActionFactory<translation_engine_action_t>().get());

    //    int result = tool.run(newFrontendActionFactory<ASTDumpAction>().get());
    if (result == 0)
    {
        return EXIT_SUCCESS;
    }
    else
    {
        return EXIT_FAILURE;
    }
}
