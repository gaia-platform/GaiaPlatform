//
// Created by simone on 8/8/21.
//

#pragma once

#include <string>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <llvm/Support/CommandLine.h>
#pragma clang diagnostic pop

#include "gaia_ast_handlers.hpp"

namespace gaia
{
namespace translation
{

extern llvm::cl::OptionCategory g_translation_engine_category;
extern llvm::cl::opt<std::string> g_translation_engine_output_option;

class translation_engine_consumer_t : public clang::ASTConsumer
{
public:
    explicit translation_engine_consumer_t(clang::ASTContext*, clang::Rewriter& r);

    void HandleTranslationUnit(clang::ASTContext& context) override;

private:
    clang::ast_matchers::MatchFinder m_matcher;
    field_get_match_handler_t m_field_get_match_handler;
    field_set_match_handler_t m_field_set_match_handler;
    rule_match_handler_t m_rule_match_handler;
    ruleset_match_handler_t m_ruleset_match_handler;
    field_unary_operator_match_handler_t m_field_unary_operator_match_handler;
    variable_declaration_match_handler_t m_variable_declaration_match_handler;
    rule_context_rule_match_handler_t m_rule_context_match_handler;
    table_call_match_handler_t m_table_call_match_handler;
    if_nomatch_match_handler_t m_if_nomatch_match_handler;
    declarative_for_match_handler_t m_declarative_for_match_handler;
    declarative_break_continue_handler_t m_declarative_break_continue_handler;
    declarative_delete_handler_t m_declarative_delete_handler;
    declarative_insert_handler_t m_declarative_insert_handler;
};

class translation_engine_action_t : public clang::ASTFrontendAction
{
public:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance& compiler, llvm::StringRef) override;

    void EndSourceFileAction() override;

private:
    clang::Rewriter m_rewriter;
};

} // namespace translation
} // namespace gaia
