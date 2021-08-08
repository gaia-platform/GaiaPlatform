//
// Created by simone on 8/8/21.
//

#pragma once

#include <string>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Rewrite/Core/Rewriter.h>
#pragma clang diagnostic pop

namespace gaia
{

namespace translation
{

class field_get_match_handler_t : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    explicit field_get_match_handler_t(clang::Rewriter& r);

    void run(const clang::ast_matchers::MatchFinder::MatchResult& result) override;

private:
    clang::Rewriter& m_rewriter;
};

class field_set_match_handler_t : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    explicit field_set_match_handler_t(clang::Rewriter& r);
    void run(const clang::ast_matchers::MatchFinder::MatchResult& result) override;

private:
    std::string convert_compound_binary_opcode(clang::BinaryOperator::Opcode op_code);

    clang::Rewriter& m_rewriter;
};

class field_unary_operator_match_handler_t : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    explicit field_unary_operator_match_handler_t(clang::Rewriter& r);
    void run(const clang::ast_matchers::MatchFinder::MatchResult& result) override;

private:
    clang::Rewriter& m_rewriter;
};

class rule_match_handler_t : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    explicit rule_match_handler_t(clang::Rewriter& r);
    void run(const clang::ast_matchers::MatchFinder::MatchResult& result) override;

private:
    clang::Rewriter& m_rewriter;
};

class ruleset_match_handler_t : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    explicit ruleset_match_handler_t(clang::Rewriter& r);
    void run(const clang::ast_matchers::MatchFinder::MatchResult& result) override;

private:
    clang::Rewriter& m_rewriter;
};

class variable_declaration_match_handler_t : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    void run(const clang::ast_matchers::MatchFinder::MatchResult& result) override;
};

class rule_context_rule_match_handler_t : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    explicit rule_context_rule_match_handler_t(clang::Rewriter& r);
    void run(const clang::ast_matchers::MatchFinder::MatchResult& result) override;

private:
    clang::Rewriter& m_rewriter;
};

// AST handler that called when a table or a tag is an argument for a function call.
class table_call_match_handler_t : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    explicit table_call_match_handler_t(clang::Rewriter& r);
    void run(const clang::ast_matchers::MatchFinder::MatchResult& result) override;

private:
    clang::Rewriter& m_rewriter;
};

// AST handler that called when a nomatch is used with if.
class if_nomatch_match_handler_t : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    explicit if_nomatch_match_handler_t(clang::Rewriter& r);

    void run(const clang::ast_matchers::MatchFinder::MatchResult& result) override;

private:
    clang::Rewriter& m_rewriter;
};

// AST handler that is called when Delete function is invoked on a table.
class declarative_delete_handler_t : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    explicit declarative_delete_handler_t(clang::Rewriter& r);
    void run(const clang::ast_matchers::MatchFinder::MatchResult& result) override;

private:
    clang::Rewriter& m_rewriter;
};

// AST handler that is called when Insert function is invoked on a table.
class declarative_insert_handler_t : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    explicit declarative_insert_handler_t(clang::Rewriter& r);
    void run(const clang::ast_matchers::MatchFinder::MatchResult& result) override;

private:
    clang::Rewriter& m_rewriter;
};

// AST handler that is called when a declarative for.
class declarative_for_match_handler_t : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    explicit declarative_for_match_handler_t(clang::Rewriter& r);
    void run(const clang::ast_matchers::MatchFinder::MatchResult& result) override;

private:
    clang::Rewriter& m_rewriter;
};

// AST handler that is called when declarative break or continue are used in the rule.
class declarative_break_continue_handler_t : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    explicit declarative_break_continue_handler_t(clang::Rewriter& r);
    void run(const clang::ast_matchers::MatchFinder::MatchResult& result) override;

private:
    clang::Rewriter& m_rewriter;
};

} // namespace translation
} // namespace gaia
