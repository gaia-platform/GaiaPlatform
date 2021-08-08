//
// Created by simone on 8/8/21.
//

#include "gaia_ast_handlers.hpp"

#include <iostream>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#include <clang/Lex/Lexer.h>
#pragma clang diagnostic pop

#include "gaiat_helpers.hpp"
#include "gaiat_state.hpp"
#include "table_navigation.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace std;

namespace gaia
{
namespace translation
{

constexpr int c_declaration_to_ruleset_offset = -2;

field_get_match_handler_t::field_get_match_handler_t(Rewriter& r)
    : m_rewriter(r){};

void field_get_match_handler_t::run(const MatchFinder::MatchResult& result)
{
    validate_table_data();
    if (g_is_generation_error)
    {
        return;
    }
    const auto* expression = result.Nodes.getNodeAs<DeclRefExpr>("fieldGet");
    const auto* member_expression = result.Nodes.getNodeAs<MemberExpr>("tableFieldGet");
    string table_name;
    string field_name;
    string variable_name;
    SourceRange expression_source_range;
    explicit_path_data_t explicit_path_data;
    bool explicit_path_present = true;
    if (expression != nullptr)
    {
        const ValueDecl* decl = expression->getDecl();
        if (decl->getType()->isStructureType())
        {
            return;
        }
        table_name = get_table_name(decl);
        field_name = decl->getName().str();
        variable_name = expression->getNameInfo().getAsString();
        if (!get_explicit_path_data(decl, explicit_path_data, expression_source_range))
        {
            variable_name = table_navigation_t::get_variable_name(table_name, explicit_path_data.tag_table_map);
            explicit_path_present = false;
            expression_source_range = SourceRange(expression->getLocation(), expression->getEndLoc());
            g_used_dbs.insert(table_navigation_t::get_table_data().find(table_name)->second.db_name);
        }
        else
        {
            variable_name = get_table_from_expression(explicit_path_data.path_components.back());
            get_variable_name(variable_name, table_name, explicit_path_data);
            update_used_dbs(explicit_path_data);
            expression_source_range.setEnd(expression->getEndLoc());
        }
        if (decl->hasAttr<GaiaFieldValueAttr>())
        {
            if (!explicit_path_present)
            {
                expression_source_range
                    = SourceRange(expression_source_range.getBegin().getLocWithOffset(-1), expression_source_range.getEnd());
            }

            if (!validate_and_add_active_field(table_name, field_name, true))
            {
                return;
            }
        }
    }
    else if (member_expression != nullptr)
    {
        auto* declaration_expression = dyn_cast<DeclRefExpr>(member_expression->getBase());
        if (declaration_expression != nullptr)
        {
            field_name = member_expression->getMemberNameInfo().getName().getAsString();
            table_name = get_table_name(declaration_expression->getDecl());
            variable_name = declaration_expression->getNameInfo().getAsString();
            const ValueDecl* decl = declaration_expression->getDecl();

            if (!get_explicit_path_data(decl, explicit_path_data, expression_source_range))
            {
                variable_name = table_navigation_t::get_variable_name(variable_name, explicit_path_data.tag_table_map);
                explicit_path_present = false;
                expression_source_range
                    = SourceRange(
                        member_expression->getBeginLoc(),
                        member_expression->getEndLoc());
                g_used_dbs.insert(table_navigation_t::get_table_data().find(table_name)->second.db_name);
            }
            else
            {
                variable_name = get_table_from_expression(explicit_path_data.path_components.back());
                get_variable_name(variable_name, table_name, explicit_path_data);
                update_used_dbs(explicit_path_data);
                expression_source_range.setEnd(member_expression->getEndLoc());
            }

            if (decl->hasAttr<GaiaFieldValueAttr>())
            {
                if (!explicit_path_present)
                {
                    expression_source_range
                        = SourceRange(
                            expression_source_range.getBegin().getLocWithOffset(-1), expression_source_range.getEnd());
                }

                if (!validate_and_add_active_field(table_name, field_name, true))
                {
                    return;
                }
            }
        }
        else
        {
            cerr << "Incorrect base type of generated type." << endl;
            g_is_generation_error = true;
            return;
        }
    }
    else
    {
        cerr << "Incorrect matched expression." << endl;
        g_is_generation_error = true;
        return;
    }
    if (expression_source_range.isValid())
    {
        g_used_dbs.insert(table_navigation_t::get_table_data().find(table_name)->second.db_name);
        m_rewriter.ReplaceText(expression_source_range, variable_name + "." + field_name + "()");
        g_rewriter_history.push_back({expression_source_range, variable_name + "." + field_name + "()", replace_text});
        auto offset
            = Lexer::MeasureTokenLength(
                  expression_source_range.getEnd(), m_rewriter.getSourceMgr(), m_rewriter.getLangOpts())
            + 1;
        if (!explicit_path_present)
        {
            update_expression_used_tables(
                result.Context,
                expression,
                table_name,
                variable_name,
                SourceRange(expression_source_range.getBegin(), expression_source_range.getEnd().getLocWithOffset(offset)),
                m_rewriter);
            update_expression_used_tables(
                result.Context,
                member_expression,
                table_name,
                variable_name,
                SourceRange(expression_source_range.getBegin(), expression_source_range.getEnd().getLocWithOffset(offset)),
                m_rewriter);
        }
        else
        {
            update_expression_explicit_path_data(
                result.Context,
                expression,
                explicit_path_data,
                SourceRange(expression_source_range.getBegin(), expression_source_range.getEnd().getLocWithOffset(offset)),
                m_rewriter);
            update_expression_explicit_path_data(
                result.Context,
                member_expression,
                explicit_path_data,
                SourceRange(expression_source_range.getBegin(), expression_source_range.getEnd().getLocWithOffset(offset)),
                m_rewriter);
        }
    }
};

field_set_match_handler_t::field_set_match_handler_t(Rewriter& r)
    : m_rewriter(r){};

void field_set_match_handler_t::run(const MatchFinder::MatchResult& result)
{
    validate_table_data();
    if (g_is_generation_error)
    {
        return;
    }
    const auto* op = result.Nodes.getNodeAs<BinaryOperator>("fieldSet");
    if (op == nullptr)
    {
        cerr << "Incorrect matched operator." << endl;
        g_is_generation_error = true;
        return;
    }
    const Expr* operator_expression = op->getLHS();
    if (operator_expression == nullptr)
    {
        cerr << "Incorrect operator expression" << endl;
        g_is_generation_error = true;
        return;
    }
    const auto* left_declaration_expression = dyn_cast<DeclRefExpr>(operator_expression);
    const auto* member_expression = dyn_cast<MemberExpr>(operator_expression);

    explicit_path_data_t explicit_path_data;
    bool explicit_path_present = true;

    string table_name;
    string field_name;
    string variable_name;
    SourceRange set_source_range;
    if (left_declaration_expression == nullptr && member_expression == nullptr)
    {
        cerr << "Incorrect operator expression type." << endl;
        g_is_generation_error = true;
        return;
    }
    if (left_declaration_expression != nullptr)
    {
        const ValueDecl* operator_declaration = left_declaration_expression->getDecl();
        if (operator_declaration->getType()->isStructureType())
        {
            return;
        }
        table_name = get_table_name(operator_declaration);
        field_name = operator_declaration->getName().str();
        variable_name = table_navigation_t::get_variable_name(table_name, unordered_map<string, string>());
        if (!get_explicit_path_data(operator_declaration, explicit_path_data, set_source_range))
        {
            explicit_path_present = false;
            set_source_range.setBegin(left_declaration_expression->getLocation());
            g_used_dbs.insert(table_navigation_t::get_table_data().find(table_name)->second.db_name);
        }
        else
        {
            variable_name = get_table_from_expression(explicit_path_data.path_components.back());
            get_variable_name(variable_name, table_name, explicit_path_data);
            update_used_dbs(explicit_path_data);
        }
    }
    else
    {
        auto* declaration_expression = dyn_cast<DeclRefExpr>(member_expression->getBase());
        if (declaration_expression == nullptr)
        {
            cerr << "Incorrect base type of generated type." << endl;
            g_is_generation_error = true;
            return;
        }
        const ValueDecl* decl = declaration_expression->getDecl();
        field_name = member_expression->getMemberNameInfo().getName().getAsString();
        table_name = get_table_name(decl);
        variable_name = declaration_expression->getNameInfo().getAsString();

        if (!get_explicit_path_data(decl, explicit_path_data, set_source_range))
        {
            variable_name = table_navigation_t::get_variable_name(variable_name, explicit_path_data.tag_table_map);
            explicit_path_present = false;
            set_source_range.setBegin(member_expression->getBeginLoc());
            g_used_dbs.insert(table_navigation_t::get_table_data().find(table_name)->second.db_name);
        }
        else
        {
            variable_name = get_table_from_expression(explicit_path_data.path_components.back());
            get_variable_name(variable_name, table_name, explicit_path_data);
            update_used_dbs(explicit_path_data);
        }
    }
    tok::TokenKind token_kind;
    string replacement_text = "[&]() mutable {auto w = " + variable_name + ".writer(); w." + field_name;

    switch (op->getOpcode())
    {
    case BO_Assign:
    {
        token_kind = tok::equal;
        break;
    }
    case BO_MulAssign:
    {
        token_kind = tok::starequal;
        break;
    }
    case BO_DivAssign:
    {
        token_kind = tok::slashequal;
        break;
    }
    case BO_RemAssign:
    {
        token_kind = tok::percentequal;
        break;
    }
    case BO_AddAssign:
    {
        token_kind = tok::plusequal;
        break;
    }
    case BO_SubAssign:
    {
        token_kind = tok::minusequal;
        break;
    }
    case BO_ShlAssign:
    {
        token_kind = tok::lesslessequal;
        break;
    }
    case BO_ShrAssign:
    {
        token_kind = tok::greatergreaterequal;
        break;
    }
    case BO_AndAssign:
    {
        token_kind = tok::ampequal;
        break;
    }
    case BO_XorAssign:
    {
        token_kind = tok::caretequal;
        break;
    }
    case BO_OrAssign:
    {
        token_kind = tok::pipeequal;
        break;
    }
    default:
        cerr << "Incorrect operator type." << endl;
        g_is_generation_error = true;
        return;
    }

    replacement_text += convert_compound_binary_opcode(op->getOpcode());

    if (left_declaration_expression != nullptr)
    {
        set_source_range.setEnd(Lexer::findLocationAfterToken(
                                    set_source_range.getBegin(), token_kind, m_rewriter.getSourceMgr(),
                                    m_rewriter.getLangOpts(), true)
                                    .getLocWithOffset(-1));
    }
    else
    {
        set_source_range.setEnd(Lexer::findLocationAfterToken(
                                    member_expression->getExprLoc(), token_kind, m_rewriter.getSourceMgr(),
                                    m_rewriter.getLangOpts(), true)
                                    .getLocWithOffset(-1));
    }
    m_rewriter.ReplaceText(set_source_range, replacement_text);
    g_rewriter_history.push_back({set_source_range, replacement_text, replace_text});
    m_rewriter.InsertTextAfterToken(
        op->getEndLoc(), "; w.update_row(); return w." + field_name + ";}()");
    g_rewriter_history.push_back(
        {SourceRange(op->getEndLoc()), "; w.update_row(); return w." + field_name + ";}()",
         insert_text_after_token});

    auto offset = Lexer::MeasureTokenLength(op->getEndLoc(), m_rewriter.getSourceMgr(), m_rewriter.getLangOpts()) + 1;
    if (!explicit_path_present)
    {
        update_expression_used_tables(
            result.Context,
            op,
            table_name,
            variable_name,
            SourceRange(set_source_range.getBegin(), op->getEndLoc().getLocWithOffset(offset)),
            m_rewriter);
    }
    else
    {
        update_expression_explicit_path_data(
            result.Context,
            op,
            explicit_path_data,
            SourceRange(set_source_range.getBegin(), op->getEndLoc().getLocWithOffset(offset)),
            m_rewriter);
    }
}

string field_set_match_handler_t::convert_compound_binary_opcode(BinaryOperator::Opcode op_code)
{
    switch (op_code)
    {
    case BO_Assign:
        return "=";
    case BO_MulAssign:
        return "*=";
    case BO_DivAssign:
        return "/=";
    case BO_RemAssign:
        return "%=";
    case BO_AddAssign:
        return "+=";
    case BO_SubAssign:
        return "-=";
    case BO_ShlAssign:
        return "<<=";
    case BO_ShrAssign:
        return ">>=";
    case BO_AndAssign:
        return "&=";
    case BO_XorAssign:
        return "^=";
    case BO_OrAssign:
        return "|=";
    default:
        cerr << "Incorrect operator code " << op_code << "." << endl;
        g_is_generation_error = true;
        return "";
    }
}

field_unary_operator_match_handler_t::field_unary_operator_match_handler_t(Rewriter& r)
    : m_rewriter(r){};

void field_unary_operator_match_handler_t::run(const MatchFinder::MatchResult& result)
{
    validate_table_data();
    if (g_is_generation_error)
    {
        return;
    }
    const auto* op = result.Nodes.getNodeAs<UnaryOperator>("fieldUnaryOp");
    if (op == nullptr)
    {
        cerr << "Incorrect matched operator." << endl;
        g_is_generation_error = true;
        return;
    }
    const Expr* operator_expression = op->getSubExpr();
    if (operator_expression == nullptr)
    {
        cerr << "Incorrect operator expression." << endl;
        g_is_generation_error = true;
        return;
    }
    const auto* declaration_expression = dyn_cast<DeclRefExpr>(operator_expression);
    const auto* member_expression = dyn_cast<MemberExpr>(operator_expression);

    if (declaration_expression == nullptr && member_expression == nullptr)
    {
        cerr << "Incorrect operator expression type." << endl;
        g_is_generation_error = true;
        return;
    }
    explicit_path_data_t explicit_path_data;
    bool explicit_path_present = true;
    string replace_string;
    string table_name;
    string field_name;
    string variable_name;
    SourceRange operator_source_range;

    if (declaration_expression != nullptr)
    {
        const ValueDecl* operator_declaration = declaration_expression->getDecl();
        if (operator_declaration->getType()->isStructureType())
        {
            return;
        }

        table_name = get_table_name(operator_declaration);
        field_name = operator_declaration->getName().str();
        variable_name = declaration_expression->getNameInfo().getAsString();
        if (!get_explicit_path_data(operator_declaration, explicit_path_data, operator_source_range))
        {
            variable_name = table_navigation_t::get_variable_name(table_name, explicit_path_data.tag_table_map);
            explicit_path_present = false;
            g_used_dbs.insert(table_navigation_t::get_table_data().find(table_name)->second.db_name);
        }
        else
        {
            variable_name = get_table_from_expression(explicit_path_data.path_components.back());
            get_variable_name(variable_name, table_name, explicit_path_data);
            update_used_dbs(explicit_path_data);
        }
    }
    else
    {
        auto* declaration_expression = dyn_cast<DeclRefExpr>(member_expression->getBase());
        if (declaration_expression == nullptr)
        {
            cerr << "Incorrect base type of generated type." << endl;
            g_is_generation_error = true;
            return;
        }
        const ValueDecl* operator_declaration = declaration_expression->getDecl();
        field_name = member_expression->getMemberNameInfo().getName().getAsString();
        table_name = get_table_name(operator_declaration);
        variable_name = declaration_expression->getNameInfo().getAsString();
        if (!get_explicit_path_data(operator_declaration, explicit_path_data, operator_source_range))
        {
            variable_name = table_navigation_t::get_variable_name(variable_name, explicit_path_data.tag_table_map);
            explicit_path_present = false;
            g_used_dbs.insert(table_navigation_t::get_table_data().find(table_name)->second.db_name);
        }
        else
        {
            variable_name = get_table_from_expression(explicit_path_data.path_components.back());
            get_variable_name(variable_name, table_name, explicit_path_data);
            update_used_dbs(explicit_path_data);
        }
    }

    if (op->isPostfix())
    {
        if (op->isIncrementOp())
        {
            replace_string
                = "[&]() mutable {auto t = "
                + variable_name + "." + field_name + "(); auto w = "
                + variable_name + ".writer(); w." + field_name + "++; w.update_row(); return t;}()";
        }
        else if (op->isDecrementOp())
        {
            replace_string
                = "[&]() mutable {auto t =" + variable_name + "." + field_name + "(); auto w = "
                + variable_name + ".writer(); w." + field_name + "--; w.update_row(); return t;}()";
        }
    }
    else
    {
        if (op->isIncrementOp())
        {
            replace_string
                = "[&]() mutable {auto w = " + variable_name + ".writer(); ++ w." + field_name
                + ";w.update_row(); return w." + field_name + ";}()";
        }
        else if (op->isDecrementOp())
        {
            replace_string
                = "[&]() mutable {auto w = " + variable_name + ".writer(); -- w." + field_name
                + ";w.update_row(); return w." + field_name + ";}()";
        }
    }
    m_rewriter.ReplaceText(
        SourceRange(op->getBeginLoc().getLocWithOffset(-1), op->getEndLoc().getLocWithOffset(1)),
        replace_string);
    g_rewriter_history.push_back(
        {SourceRange(op->getBeginLoc().getLocWithOffset(-1), op->getEndLoc().getLocWithOffset(1)),
         replace_string, replace_text});
    auto offset = Lexer::MeasureTokenLength(op->getEndLoc(), m_rewriter.getSourceMgr(), m_rewriter.getLangOpts()) + 1;

    if (!explicit_path_present)
    {
        update_expression_used_tables(
            result.Context,
            op,
            table_name,
            variable_name,
            SourceRange(op->getBeginLoc().getLocWithOffset(-1), op->getEndLoc().getLocWithOffset(offset)),
            m_rewriter);
    }
    else
    {
        if (op->isPrefix())
        {
            offset += 1;
        }
        update_expression_explicit_path_data(
            result.Context,
            op,
            explicit_path_data,
            SourceRange(op->getBeginLoc().getLocWithOffset(-1), op->getEndLoc().getLocWithOffset(offset)),
            m_rewriter);
    }
}
rule_match_handler_t::rule_match_handler_t(Rewriter& r)
    : m_rewriter(r){};

void rule_match_handler_t::run(const MatchFinder::MatchResult& result)
{
    if (g_is_generation_error)
    {
        return;
    }

    const auto* rule_declaration = result.Nodes.getNodeAs<FunctionDecl>("ruleDecl");
    const GaiaOnUpdateAttr* update_attribute = rule_declaration->getAttr<GaiaOnUpdateAttr>();
    const GaiaOnInsertAttr* insert_attribute = rule_declaration->getAttr<GaiaOnInsertAttr>();
    const GaiaOnChangeAttr* change_attribute = rule_declaration->getAttr<GaiaOnChangeAttr>();

    generate_rules(m_rewriter);
    if (g_is_generation_error)
    {
        return;
    }

    if (g_current_rule_declaration)
    {
        g_current_ruleset_rule_number++;
    }

    g_current_rule_declaration = rule_declaration;

    SourceRange rule_range = g_current_rule_declaration->getSourceRange();
    g_current_ruleset_rule_line_number = m_rewriter.getSourceMgr().getSpellingLineNumber(rule_range.getBegin());
    g_expression_explicit_path_data.clear();
    g_insert_tables.clear();
    g_update_tables.clear();
    g_active_fields.clear();
    g_attribute_tag_map.clear();
    g_rewriter_history.clear();
    g_nomatch_location.clear();
    g_nomatch_location_map.clear();
    g_variable_declaration_location.clear();
    g_variable_declaration_init_location.clear();
    g_is_rule_prolog_specified = false;
    g_rule_attribute_source_range = SourceRange();
    g_is_rule_context_rule_name_referenced = false;

    if (update_attribute != nullptr)
    {
        g_rule_attribute_source_range = update_attribute->getRange();
        g_is_rule_prolog_specified = true;
        for (const auto& table_iterator : update_attribute->tables())
        {
            string table, field, tag;
            if (parse_attribute(table_iterator, table, field, tag))
            {
                if (field.empty())
                {
                    g_update_tables.insert(table);
                }
                else
                {
                    if (!validate_and_add_active_field(table, field))
                    {
                        return;
                    }
                }

                if (!tag.empty())
                {
                    g_attribute_tag_map[tag] = table;
                }
            }
        }
    }

    if (insert_attribute != nullptr)
    {
        g_is_rule_prolog_specified = true;
        g_rule_attribute_source_range = insert_attribute->getRange();
        for (const auto& table_iterator : insert_attribute->tables())
        {
            string table, field, tag;
            if (parse_attribute(table_iterator, table, field, tag))
            {
                g_insert_tables.insert(table);
                if (!tag.empty())
                {
                    g_attribute_tag_map[tag] = table;
                }
            }
        }
    }

    if (change_attribute != nullptr)
    {
        g_is_rule_prolog_specified = true;
        g_rule_attribute_source_range = change_attribute->getRange();
        for (const auto& table_iterator : change_attribute->tables())
        {
            string table, field, tag;
            if (parse_attribute(table_iterator, table, field, tag))
            {
                g_insert_tables.insert(table);
                if (field.empty())
                {
                    g_update_tables.insert(table);
                }
                else
                {
                    if (!validate_and_add_active_field(table, field))
                    {
                        return;
                    }
                }
                if (!tag.empty())
                {
                    g_attribute_tag_map[tag] = table;
                }
            }
        }
    }
}

ruleset_match_handler_t::ruleset_match_handler_t(Rewriter& r)
    : m_rewriter(r){};

void ruleset_match_handler_t::run(const MatchFinder::MatchResult& result)
{
    if (g_is_generation_error)
    {
        return;
    }
    generate_rules(m_rewriter);
    if (g_is_generation_error)
    {
        return;
    }
    g_current_rule_declaration = nullptr;
    g_expression_explicit_path_data.clear();
    g_active_fields.clear();
    g_insert_tables.clear();
    g_update_tables.clear();
    g_attribute_tag_map.clear();
    g_rewriter_history.clear();
    g_nomatch_location.clear();
    g_nomatch_location_map.clear();
    g_variable_declaration_location.clear();
    g_variable_declaration_init_location.clear();
    g_is_rule_prolog_specified = false;
    g_rule_attribute_source_range = SourceRange();

    const auto* ruleset_declaration = result.Nodes.getNodeAs<RulesetDecl>("rulesetDecl");
    if (ruleset_declaration == nullptr)
    {
        return;
    }
    if (!g_current_ruleset.empty())
    {
        g_generated_subscription_code
            += "\nnamespace " + g_current_ruleset
            + "{\nvoid subscribe_ruleset_" + g_current_ruleset
            + "()\n{\n" + g_current_ruleset_subscription
            + "}\nvoid unsubscribe_ruleset_" + g_current_ruleset
            + "()\n{\n" + g_current_ruleset_unsubscription + "}\n}\n";
    }
    g_current_ruleset = ruleset_declaration->getName().str();

    // Make sure each new ruleset name is unique.
    for (const auto& r : g_rulesets)
    {
        if (r == g_current_ruleset)
        {
            cerr << "Ruleset names must be unique - '"
                 << g_current_ruleset
                 << "' has been found multiple times." << endl;
            g_is_generation_error = true;
            return;
        }
    }

    g_rulesets.push_back(g_current_ruleset);
    g_current_ruleset_subscription.clear();
    g_current_ruleset_unsubscription.clear();
    g_current_ruleset_rule_number = 1;
    if (*(ruleset_declaration->decls_begin()) == nullptr)
    {
        // Empty ruleset so it doesn't make sense to process any possible attributes
        m_rewriter.ReplaceText(
            SourceRange(
                ruleset_declaration->getBeginLoc(),
                ruleset_declaration->getEndLoc()),
            "namespace " + g_current_ruleset
                + "\n{\n} // namespace " + g_current_ruleset + "\n");
        g_rewriter_history.push_back(
            {SourceRange(ruleset_declaration->getBeginLoc(), ruleset_declaration->getEndLoc()),
             "namespace " + g_current_ruleset + "\n{\n} // namespace " + g_current_ruleset + "\n",
             replace_text});
    }
    else
    {
        // Replace ruleset declaration that may include attributes with namespace declaration
        m_rewriter.ReplaceText(
            SourceRange(
                ruleset_declaration->getBeginLoc(),
                ruleset_declaration->decls_begin()->getBeginLoc().getLocWithOffset(c_declaration_to_ruleset_offset)),
            "namespace " + g_current_ruleset + "\n{\n");

        // Replace closing brace with namespace comment.
        m_rewriter.ReplaceText(SourceRange(ruleset_declaration->getEndLoc()), "}// namespace " + g_current_ruleset);

        g_rewriter_history.push_back(
            {SourceRange(ruleset_declaration->getBeginLoc(), ruleset_declaration->decls_begin()->getBeginLoc().getLocWithOffset(c_declaration_to_ruleset_offset)),
             "namespace " + g_current_ruleset + "\n{\n", replace_text});
        g_rewriter_history.push_back(
            {SourceRange(ruleset_declaration->getEndLoc()),
             "}// namespace " + g_current_ruleset, replace_text});
    }
}

void variable_declaration_match_handler_t::run(const MatchFinder::MatchResult& result)
{
    if (g_is_generation_error)
    {
        return;
    }
    validate_table_data();
    const auto* variable_declaration = result.Nodes.getNodeAs<VarDecl>("varDeclaration");
    const auto* variable_declaration_init = result.Nodes.getNodeAs<VarDecl>("varDeclarationInit");
    if (variable_declaration_init != nullptr)
    {
        g_variable_declaration_init_location.insert(variable_declaration_init->getSourceRange());
    }
    if (variable_declaration == nullptr)
    {
        return;
    }
    const auto variable_name = variable_declaration->getNameAsString();
    if (variable_name != "")
    {
        g_variable_declaration_location[variable_declaration->getSourceRange()] = variable_name;

        if (table_navigation_t::get_table_data().find(variable_name) != table_navigation_t::get_table_data().end())
        {
            cerr << "Local variable declaration '" << variable_name
                 << "' hides database table of the same name." << endl;
            return;
        }

        for (auto table_data : table_navigation_t::get_table_data())
        {
            if (table_data.second.field_data.find(variable_name) != table_data.second.field_data.end())
            {
                cerr << "Local variable declaration '" << variable_name
                     << "' hides catalog field entity of the same name." << endl;
                return;
            }
        }
    }
}

rule_context_rule_match_handler_t::rule_context_rule_match_handler_t(Rewriter& r)
    : m_rewriter(r){};

void rule_context_rule_match_handler_t::run(const MatchFinder::MatchResult& result)
{
    if (g_is_generation_error)
    {
        return;
    }

    const auto* rule_expression = result.Nodes.getNodeAs<MemberExpr>("rule_name");
    const auto* ruleset_expression = result.Nodes.getNodeAs<MemberExpr>("ruleset_name");
    const auto* event_expression = result.Nodes.getNodeAs<MemberExpr>("event_type");
    const auto* type_expression = result.Nodes.getNodeAs<MemberExpr>("gaia_type");

    if (ruleset_expression != nullptr)
    {
        m_rewriter.ReplaceText(
            SourceRange(ruleset_expression->getBeginLoc(), ruleset_expression->getEndLoc()),
            "\"" + g_current_ruleset + "\"");
        g_rewriter_history.push_back(
            {SourceRange(ruleset_expression->getBeginLoc(), ruleset_expression->getEndLoc()),
             "\"" + g_current_ruleset + "\"", replace_text});
    }

    if (rule_expression != nullptr)
    {
        m_rewriter.ReplaceText(
            SourceRange(rule_expression->getBeginLoc(), rule_expression->getEndLoc()),
            "gaia_rule_name");
        g_is_rule_context_rule_name_referenced = true;
        g_rewriter_history.push_back(
            {SourceRange(rule_expression->getBeginLoc(), rule_expression->getEndLoc()),
             "gaia_rule_name", replace_text});
    }

    if (event_expression != nullptr)
    {
        m_rewriter.ReplaceText(
            SourceRange(event_expression->getBeginLoc(), event_expression->getEndLoc()),
            "context->event_type");
        g_rewriter_history.push_back(
            {SourceRange(event_expression->getBeginLoc(), event_expression->getEndLoc()),
             "context->event_type", replace_text});
    }

    if (type_expression != nullptr)
    {
        m_rewriter.ReplaceText(
            SourceRange(type_expression->getBeginLoc(), type_expression->getEndLoc()),
            "context->gaia_type");
        g_rewriter_history.push_back(
            {SourceRange(type_expression->getBeginLoc(), type_expression->getEndLoc()),
             "context->gaia_type", replace_text});
    }
}

table_call_match_handler_t::table_call_match_handler_t(Rewriter& r)
    : m_rewriter(r){};

void table_call_match_handler_t::run(const MatchFinder::MatchResult& result)
{
    validate_table_data();
    if (g_is_generation_error)
    {
        return;
    }
    const auto* expression = result.Nodes.getNodeAs<DeclRefExpr>("tableCall");

    if (expression == nullptr)
    {
        cerr << "Incorrect matched expression." << endl;
        g_is_generation_error = true;
        return;
    }
    string table_name;
    SourceRange expression_source_range;
    explicit_path_data_t explicit_path_data;
    bool explicit_path_present = true;
    string variable_name;
    const ValueDecl* decl = expression->getDecl();
    if (!decl->getType()->isStructureType())
    {
        return;
    }
    table_name = get_table_name(decl);
    variable_name = decl->getNameAsString();
    if (!get_explicit_path_data(decl, explicit_path_data, expression_source_range))
    {
        variable_name = table_navigation_t::get_variable_name(table_name, explicit_path_data.tag_table_map);
        explicit_path_present = false;
        expression_source_range = SourceRange(expression->getLocation(), expression->getEndLoc());
        g_used_dbs.insert(table_navigation_t::get_table_data().find(table_name)->second.db_name);
    }
    else
    {
        variable_name = get_table_from_expression(explicit_path_data.path_components.back());
        get_variable_name(variable_name, table_name, explicit_path_data);
        update_used_dbs(explicit_path_data);
        expression_source_range
            = SourceRange(expression_source_range.getBegin(), expression_source_range.getEnd().getLocWithOffset(-1));
    }

    if (decl->hasAttr<GaiaFieldValueAttr>())
    {
        expression_source_range
            = SourceRange(expression_source_range.getBegin().getLocWithOffset(-1), expression_source_range.getEnd());
    }

    if (expression_source_range.isValid())
    {
        if (g_insert_call_locations.find(expression->getBeginLoc()) != g_insert_call_locations.end())
        {
            if (explicit_path_present)
            {
                cerr << "'insert' call cannot be used with navigation." << endl;
                g_is_generation_error = true;
                return;
            }

            if (table_name == variable_name)
            {
                cerr << "'insert' call cannot be used with tags." << endl;
                g_is_generation_error = true;
                return;
            }
            return;
        }
        m_rewriter.ReplaceText(expression_source_range, variable_name);
        g_rewriter_history.push_back({expression_source_range, variable_name, replace_text});

        auto offset
            = Lexer::MeasureTokenLength(expression_source_range.getEnd(), m_rewriter.getSourceMgr(), m_rewriter.getLangOpts()) + 1;
        if (explicit_path_present)
        {
            update_expression_explicit_path_data(
                result.Context,
                expression,
                explicit_path_data,
                SourceRange(expression_source_range.getBegin(), expression_source_range.getEnd().getLocWithOffset(offset)),
                m_rewriter);
        }
        else
        {
            update_expression_used_tables(
                result.Context,
                expression,
                table_name,
                variable_name,
                SourceRange(expression_source_range.getBegin(), expression_source_range.getEnd().getLocWithOffset(offset)),
                m_rewriter);
        }
    }
}

if_nomatch_match_handler_t::if_nomatch_match_handler_t(Rewriter& r)
    : m_rewriter(r){};

void if_nomatch_match_handler_t::run(const MatchFinder::MatchResult& result)
{
    if (g_is_generation_error)
    {
        return;
    }
    const auto* expression = result.Nodes.getNodeAs<IfStmt>("NoMatchIf");
    if (expression != nullptr)
    {
        SourceRange nomatch_location = get_statement_source_range(expression->getNoMatch(), m_rewriter.getSourceMgr(), m_rewriter.getLangOpts());
        g_nomatch_location_map[nomatch_location] = expression->getNoMatchLoc();
        g_nomatch_location.emplace_back(nomatch_location);
    }
    else
    {
        cerr << "Incorrect matched expression." << endl;
        g_is_generation_error = true;
    }
}

declarative_delete_handler_t::declarative_delete_handler_t(Rewriter& r)
    : m_rewriter(r){};

void declarative_delete_handler_t::run(const MatchFinder::MatchResult& result)
{
    if (g_is_generation_error)
    {
        return;
    }
    const auto* expression = result.Nodes.getNodeAs<CXXMemberCallExpr>("RemoveCall");
    if (expression != nullptr)
    {
        m_rewriter.ReplaceText(SourceRange(expression->getExprLoc(), expression->getEndLoc()), "delete_row()");
        g_rewriter_history.push_back({SourceRange(expression->getExprLoc(), expression->getEndLoc()), "delete_row()", replace_text});
    }
    else
    {
        cerr << "Incorrect matched expression." << endl;
        g_is_generation_error = true;
    }
}

declarative_insert_handler_t::declarative_insert_handler_t(Rewriter& r)
    : m_rewriter(r){};

void declarative_insert_handler_t::run(const MatchFinder::MatchResult& result)
{
    if (g_is_generation_error)
    {
        return;
    }
    const auto* expression = result.Nodes.getNodeAs<CXXMemberCallExpr>("InsertCall");
    const auto* expression_declaration = result.Nodes.getNodeAs<DeclRefExpr>("tableCall");
    if (expression == nullptr || expression_declaration == nullptr)
    {
        cerr << "Incorrect matched expression." << endl;
        g_is_generation_error = true;
        return;
    }
    SourceLocation argument_start_location = expression->getExprLoc();
    unordered_map<string, string> argument_map;
    const ValueDecl* decl = expression_declaration->getDecl();
    string table_name = get_table_name(decl);

    // Parse insert call arguments to buid name value map.
    for (auto argument : expression->arguments())
    {
        string raw_argument_name = m_rewriter.getRewrittenText(
            SourceRange(argument_start_location, argument->getSourceRange().getBegin().getLocWithOffset(-1)));
        size_t argument_name_start_position = raw_argument_name.find(',');
        if (argument_name_start_position == string::npos)
        {
            argument_name_start_position = raw_argument_name.find('(');
        }
        size_t argument_name_end_position = raw_argument_name.find(':');
        string argument_name = raw_argument_name.substr(
            argument_name_start_position + 1, argument_name_end_position - argument_name_start_position - 1);
        // Trim the argument name of whitespaces.
        argument_name.erase(argument_name.begin(), find_if(argument_name.begin(), argument_name.end(), [](unsigned char ch) { return !isspace(ch); }));
        argument_name.erase(find_if(argument_name.rbegin(), argument_name.rend(), [](unsigned char ch) { return !isspace(ch); }).base(), argument_name.end());
        argument_map[argument_name] = m_rewriter.getRewrittenText(argument->getSourceRange());

        argument_start_location = argument->getSourceRange().getEnd().getLocWithOffset(1);
    }
    string class_qualification_string = "gaia::";
    class_qualification_string
        .append(table_navigation_t::get_table_data().find(table_name)->second.db_name)
        .append("::")
        .append(table_name)
        .append("_t::");
    string replacement_string = class_qualification_string;
    replacement_string
        .append("get(")
        .append(class_qualification_string)
        .append("insert_row(");
    vector<string> function_arguments = table_navigation_t::get_table_fields(table_name);
    const auto table_data_iterator = table_navigation_t::get_table_data().find(table_name);
    // Generate call arguments.
    for (const auto& call_argument : function_arguments)
    {
        const auto argument_map_iterator = argument_map.find(call_argument);
        if (argument_map_iterator == argument_map.end())
        {
            // Provide default parameter value.
            const auto field_data_iterator = table_data_iterator->second.field_data.find(call_argument);
            switch (static_cast<data_type_t>(field_data_iterator->second.field_type))
            {
            case data_type_t::e_bool:
                replacement_string.append("false,");
                break;
            case data_type_t::e_int8:
            case data_type_t::e_uint8:
            case data_type_t::e_int16:
            case data_type_t::e_uint16:
            case data_type_t::e_int32:
            case data_type_t::e_uint32:
            case data_type_t::e_int64:
            case data_type_t::e_uint64:
            case data_type_t::e_float:
            case data_type_t::e_double:
                replacement_string.append("0,");
                break;
            case data_type_t::e_string:
                replacement_string.append("\"\",");
                break;
            }
        }
        else
        {
            // Provide value from the code.
            replacement_string.append(argument_map_iterator->second).append(",");
        }
    }
    replacement_string.resize(replacement_string.size() - 1);
    replacement_string.append("))");
    cerr << replacement_string << endl;
    m_rewriter.ReplaceText(SourceRange(expression->getBeginLoc(), expression->getEndLoc()), replacement_string);
    g_rewriter_history.push_back({SourceRange(expression->getBeginLoc(), expression->getEndLoc()), replacement_string, replace_text});
    g_insert_call_locations.insert(expression->getBeginLoc());
}

declarative_for_match_handler_t::declarative_for_match_handler_t(Rewriter& r)
    : m_rewriter(r){};

void declarative_for_match_handler_t::run(const MatchFinder::MatchResult& result)
{
    if (g_is_generation_error)
    {
        return;
    }
    const auto* expression = result.Nodes.getNodeAs<GaiaForStmt>("DeclFor");
    if (expression == nullptr)
    {
        cerr << "Incorrect matched expression." << endl;
        g_is_generation_error = true;
        return;
    }

    string table_name;
    string variable_name;
    SourceRange expression_source_range;
    explicit_path_data_t explicit_path_data;
    bool explicit_path_present = true;

    const auto* path = dyn_cast<DeclRefExpr>(expression->getPath());
    if (path == nullptr)
    {
        cerr << "Incorrect expression is used as a path in for statement." << endl;
        g_is_generation_error = true;
        return;
    }
    const ValueDecl* decl = path->getDecl();
    table_name = get_table_name(decl);
    if (!get_explicit_path_data(decl, explicit_path_data, expression_source_range))
    {
        variable_name = table_navigation_t::get_variable_name(table_name, explicit_path_data.tag_table_map);
        g_used_dbs.insert(table_navigation_t::get_table_data().find(table_name)->second.db_name);
        explicit_path_present = false;
        expression_source_range.setBegin(expression->getLParenLoc().getLocWithOffset(1));
    }
    else
    {
        variable_name = get_table_from_expression(explicit_path_data.path_components.back());
        get_variable_name(variable_name, table_name, explicit_path_data);
        update_used_dbs(explicit_path_data);
    }
    expression_source_range.setEnd(expression->getRParenLoc().getLocWithOffset(-1));

    if (expression_source_range.isValid())
    {
        if (explicit_path_present)
        {
            update_expression_explicit_path_data(
                result.Context,
                path,
                explicit_path_data,
                expression_source_range,
                m_rewriter);
        }
        else
        {
            update_expression_used_tables(
                result.Context,
                path,
                table_name,
                variable_name,
                expression_source_range,
                m_rewriter);
        }
    }
    m_rewriter.RemoveText(SourceRange(expression->getForLoc(), expression->getRParenLoc()));
    g_rewriter_history.push_back({SourceRange(expression->getForLoc(), expression->getRParenLoc()), "", remove_text});
    if (expression->getNoMatch() != nullptr)
    {
        SourceRange nomatch_location = get_statement_source_range(expression->getNoMatch(), m_rewriter.getSourceMgr(), m_rewriter.getLangOpts());
        g_nomatch_location_map[nomatch_location] = expression->getNoMatchLoc();
        g_nomatch_location.emplace_back(nomatch_location);
    }
}

declarative_break_continue_handler_t::declarative_break_continue_handler_t(Rewriter& r)
    : m_rewriter(r){};

void declarative_break_continue_handler_t::run(const MatchFinder::MatchResult& result)
{
    if (g_is_generation_error)
    {
        return;
    }
    const auto* break_expression = result.Nodes.getNodeAs<BreakStmt>("DeclBreak");
    const auto* continue_expression = result.Nodes.getNodeAs<ContinueStmt>("DeclContinue");
    if (break_expression == nullptr && continue_expression == nullptr)
    {
        cerr << "Incorrect matched expression." << endl;
        g_is_generation_error = true;
        return;
    }
    LabelDecl* decl;
    SourceRange expression_source_range;
    if (break_expression != nullptr)
    {
        decl = break_expression->getLabel();
        expression_source_range = break_expression->getSourceRange();
    }
    else
    {
        decl = continue_expression->getLabel();
        expression_source_range = continue_expression->getSourceRange();
    }

    // Handle non-declarative break/continue.
    if (decl == nullptr)
    {
        return;
    }

    const LabelStmt* label_statement = decl->getStmt();
    if (label_statement == nullptr)
    {
        g_is_generation_error = true;
        return;
    }

    const Stmt* statement = label_statement->getSubStmt();
    if (statement == nullptr)
    {
        g_is_generation_error = true;
        return;
    }
    SourceRange statement_source_range = get_statement_source_range(statement, m_rewriter.getSourceMgr(), m_rewriter.getLangOpts());

    string label_name;
    if (break_expression != nullptr)
    {
        auto break_label_iterator = g_break_label_map.find(statement_source_range);
        if (break_label_iterator == g_break_label_map.end())
        {
            label_name = table_navigation_t::get_variable_name("break", unordered_map<string, string>());
            g_break_label_map[statement_source_range] = label_name;
        }
        else
        {
            label_name = break_label_iterator->second;
        }
    }
    else
    {
        auto continue_label_iterator = g_continue_label_map.find(statement_source_range);
        if (continue_label_iterator == g_continue_label_map.end())
        {
            label_name = table_navigation_t::get_variable_name("continue", unordered_map<string, string>());
            g_continue_label_map[statement_source_range] = label_name;
        }
        else
        {
            label_name = continue_label_iterator->second;
        }
    }

    auto offset
        = Lexer::MeasureTokenLength(expression_source_range.getEnd(), m_rewriter.getSourceMgr(), m_rewriter.getLangOpts()) + 1;
    expression_source_range.setEnd(expression_source_range.getEnd().getLocWithOffset(offset));
    string replacement_string = "goto " + label_name;
    m_rewriter.ReplaceText(expression_source_range, replacement_string);
    g_rewriter_history.push_back({expression_source_range, replacement_string, replace_text});
}

} // namespace translation
} // namespace gaia
