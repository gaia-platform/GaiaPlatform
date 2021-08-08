//
// Created by simone on 8/8/21.
//

#include "gaia_ast_frontend_action.hpp"

#include "gaiat_helpers.hpp"
#include "gaiat_state.hpp"

using namespace clang;
using namespace clang::ast_matchers;
using namespace llvm;

namespace gaia
{
namespace translation
{

cl::OptionCategory g_translation_engine_category("Use translation engine options");
cl::opt<std::string> g_translation_engine_output_option(
    "output", cl::init(""), cl::desc("output file name"), cl::cat(g_translation_engine_category));

translation_engine_consumer_t::translation_engine_consumer_t(ASTContext*, Rewriter& r)
    : m_field_get_match_handler(r)
    , m_field_set_match_handler(r)
    , m_rule_match_handler(r)
    , m_ruleset_match_handler(r)
    , m_field_unary_operator_match_handler(r)
    , m_rule_context_match_handler(r)
    , m_table_call_match_handler(r)
    , m_if_nomatch_match_handler(r)
    , m_declarative_for_match_handler(r)
    , m_declarative_break_continue_handler(r)
    , m_declarative_delete_handler(r)
    , m_declarative_insert_handler(r)
{
    DeclarationMatcher ruleset_matcher = rulesetDecl().bind("rulesetDecl");
    DeclarationMatcher rule_matcher
        = functionDecl(allOf(
                           hasAncestor(ruleset_matcher),
                           hasAttr(attr::Rule)))
              .bind("ruleDecl");
    StatementMatcher ruleset_name_matcher
        = memberExpr(
              hasAncestor(ruleset_matcher),
              hasDescendant(gaiaRuleContextExpr()),
              member(hasName("ruleset_name")))
              .bind("ruleset_name");
    StatementMatcher rule_name_matcher
        = memberExpr(
              hasAncestor(ruleset_matcher),
              hasDescendant(gaiaRuleContextExpr()),
              member(hasName("rule_name")))
              .bind("rule_name");
    StatementMatcher event_type_matcher
        = memberExpr(
              hasAncestor(ruleset_matcher),
              hasDescendant(gaiaRuleContextExpr()),
              member(hasName("event_type")))
              .bind("event_type");
    StatementMatcher gaia_type_matcher
        = memberExpr(
              hasAncestor(ruleset_matcher),
              hasDescendant(gaiaRuleContextExpr()),
              member(hasName("gaia_type")))
              .bind("gaia_type");

    DeclarationMatcher variable_declaration_matcher = varDecl(hasAncestor(rule_matcher)).bind("varDeclaration");

    StatementMatcher field_get_matcher
        = declRefExpr(to(varDecl(
                          anyOf(
                              hasAttr(attr::GaiaField),
                              hasAttr(attr::FieldTable),
                              hasAttr(attr::GaiaFieldValue)),
                          unless(hasAttr(attr::GaiaFieldLValue)))))
              .bind("fieldGet");
    StatementMatcher table_call_matcher
        = declRefExpr(allOf(
                          to(varDecl(
                              anyOf(
                                  hasAttr(attr::GaiaField),
                                  hasAttr(attr::FieldTable),
                                  hasAttr(attr::GaiaFieldValue)),
                              unless(hasAttr(attr::GaiaFieldLValue)))),
                          allOf(
                              unless(
                                  hasAncestor(
                                      memberExpr(
                                          member(
                                              allOf(
                                                  hasAttr(attr::GaiaField), unless(hasAttr(attr::GaiaFieldLValue))))))),
                              anyOf(
                                  hasAncestor(callExpr()), hasAncestor(cxxMemberCallExpr())))))
              .bind("tableCall");

    StatementMatcher field_set_matcher
        = binaryOperator(
              allOf(
                  hasAncestor(ruleset_matcher),
                  isAssignmentOperator(),
                  hasLHS(declRefExpr(to(varDecl(hasAttr(attr::GaiaFieldLValue)))))))
              .bind("fieldSet");
    StatementMatcher field_unary_operator_matcher
        = unaryOperator(
              allOf(
                  hasAncestor(ruleset_matcher),
                  anyOf(
                      hasOperatorName("++"),
                      hasOperatorName("--")),
                  hasUnaryOperand(declRefExpr(to(varDecl(hasAttr(attr::GaiaFieldLValue)))))))
              .bind("fieldUnaryOp");
    StatementMatcher table_field_get_matcher
        = memberExpr(
              member(
                  allOf(
                      hasAttr(attr::GaiaField),
                      unless(hasAttr(attr::GaiaFieldLValue)))),
              hasDescendant(declRefExpr(
                  to(varDecl(
                      anyOf(
                          hasAttr(attr::GaiaField),
                          hasAttr(attr::FieldTable),
                          hasAttr(attr::GaiaFieldValue)),
                      unless(hasAttr(attr::GaiaFieldLValue)))))))
              .bind("tableFieldGet");
    StatementMatcher table_field_set_matcher
        = binaryOperator(
              allOf(
                  hasAncestor(ruleset_matcher),
                  isAssignmentOperator(),
                  hasLHS(
                      memberExpr(
                          hasDescendant(
                              declRefExpr(
                                  to(varDecl(hasAttr(attr::GaiaFieldLValue)))))))))
              .bind("fieldSet");
    StatementMatcher table_field_unary_operator_matcher
        = unaryOperator(allOf(
                            hasAncestor(ruleset_matcher),
                            anyOf(
                                hasOperatorName("++"),
                                hasOperatorName("--")),
                            hasUnaryOperand(memberExpr(member(hasAttr(attr::GaiaFieldLValue))))))
              .bind("fieldUnaryOp");

    StatementMatcher if_no_match_matcher
        = ifStmt(allOf(
                     hasAncestor(rule_matcher),
                     hasNoMatch(anything())))
              .bind("NoMatchIf");

    StatementMatcher declarative_for_matcher
        = gaiaForStmt().bind("DeclFor");

    DeclarationMatcher variable_declaration_init_matcher
        = varDecl(allOf(
                      hasAncestor(rule_matcher),
                      hasInitializer(anyOf(
                          hasDescendant(field_get_matcher),
                          hasDescendant(field_unary_operator_matcher),
                          hasDescendant(table_field_get_matcher),
                          hasDescendant(table_field_unary_operator_matcher)))))
              .bind("varDeclarationInit");

    StatementMatcher declarative_break_matcher = breakStmt().bind("DeclBreak");
    StatementMatcher declarative_continue_matcher = continueStmt().bind("DeclContinue");
    StatementMatcher declarative_delete_matcher
        = cxxMemberCallExpr(
              hasAncestor(ruleset_matcher),
              callee(cxxMethodDecl(hasName("remove"))),
              hasDescendant(table_call_matcher))
              .bind("RemoveCall");

    StatementMatcher declarative_insert_matcher
        = cxxMemberCallExpr(
              hasAncestor(ruleset_matcher),
              callee(cxxMethodDecl(hasName("insert"))),
              hasDescendant(table_call_matcher))
              .bind("InsertCall");

    m_matcher.addMatcher(field_get_matcher, &m_field_get_match_handler);
    m_matcher.addMatcher(table_field_get_matcher, &m_field_get_match_handler);

    m_matcher.addMatcher(field_set_matcher, &m_field_set_match_handler);
    m_matcher.addMatcher(table_field_set_matcher, &m_field_set_match_handler);

    m_matcher.addMatcher(rule_matcher, &m_rule_match_handler);
    m_matcher.addMatcher(ruleset_matcher, &m_ruleset_match_handler);
    m_matcher.addMatcher(field_unary_operator_matcher, &m_field_unary_operator_match_handler);

    m_matcher.addMatcher(table_field_unary_operator_matcher, &m_field_unary_operator_match_handler);

    m_matcher.addMatcher(variable_declaration_matcher, &m_variable_declaration_match_handler);
    m_matcher.addMatcher(variable_declaration_init_matcher, &m_variable_declaration_match_handler);
    m_matcher.addMatcher(ruleset_name_matcher, &m_rule_context_match_handler);
    m_matcher.addMatcher(rule_name_matcher, &m_rule_context_match_handler);
    m_matcher.addMatcher(event_type_matcher, &m_rule_context_match_handler);
    m_matcher.addMatcher(gaia_type_matcher, &m_rule_context_match_handler);
    m_matcher.addMatcher(table_call_matcher, &m_table_call_match_handler);
    m_matcher.addMatcher(if_no_match_matcher, &m_if_nomatch_match_handler);
    m_matcher.addMatcher(declarative_for_matcher, &m_declarative_for_match_handler);
    m_matcher.addMatcher(declarative_break_matcher, &m_declarative_break_continue_handler);
    m_matcher.addMatcher(declarative_continue_matcher, &m_declarative_break_continue_handler);
    m_matcher.addMatcher(declarative_delete_matcher, &m_declarative_delete_handler);
    m_matcher.addMatcher(declarative_insert_matcher, &m_declarative_insert_handler);
}

void translation_engine_consumer_t::HandleTranslationUnit(clang::ASTContext& context)
{
    m_matcher.matchAST(context);
}

std::unique_ptr<ASTConsumer> translation_engine_action_t::CreateASTConsumer(clang::CompilerInstance& compiler, llvm::StringRef)
{
    m_rewriter.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());
    return std::unique_ptr<clang::ASTConsumer>(
        new translation_engine_consumer_t(&compiler.getASTContext(), m_rewriter));
}

void translation_engine_action_t::EndSourceFileAction()
{
    if (!g_translation_engine_output_option.empty())
    {
        std::remove(g_translation_engine_output_option.c_str());
    }
    generate_rules(m_rewriter);
    if (g_is_generation_error)
    {
        return;
    }
    g_generated_subscription_code
        += "namespace " + g_current_ruleset
        + "{\nvoid subscribe_ruleset_" + g_current_ruleset + "()\n{\n" + g_current_ruleset_subscription + "}\n"
        + "void unsubscribe_ruleset_" + g_current_ruleset + "()\n{\n" + g_current_ruleset_unsubscription + "}\n"
        + "} // namespace " + g_current_ruleset + "\n"
        + generate_general_subscription_code();

    if (!shouldEraseOutputFiles() && !g_is_generation_error && !g_translation_engine_output_option.empty())
    {
        std::error_code error_code;
        llvm::raw_fd_ostream output_file(g_translation_engine_output_option, error_code, llvm::sys::fs::F_None);

        if (!output_file.has_error())
        {
            output_file << "#include <cstring>\n";
            output_file << "\n";
            output_file << "#include \"gaia/common.hpp\"\n";
            output_file << "#include \"gaia/events.hpp\"\n";
            output_file << "#include \"gaia/rules/rules.hpp\"\n";
            output_file << "\n";
            for (const string& db : g_used_dbs)
            {
                output_file << "#include \"gaia_" << db << ".h\"\n";
            }

            m_rewriter.getEditBuffer(m_rewriter.getSourceMgr().getMainFileID())
                .write(output_file);
            output_file << g_generated_subscription_code;
        }

        output_file.close();
    }
}

} // namespace translation
} // namespace gaia
