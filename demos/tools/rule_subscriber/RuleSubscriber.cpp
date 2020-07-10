/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <unordered_set>
#include <vector>

#include "clang/Driver/Options.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;

cl::OptionCategory RuleSubscriberCategory("Use Rule Subscriber options");
cl::opt<string> RuleSubscriberOutputOption("output", cl::init(""), 
    cl::desc("output file name"), cl::cat(RuleSubscriberCategory));
cl::opt<bool> RuleSubscriberVerboseOption("v",
    cl::desc("print parse tokens"), cl::cat(RuleSubscriberCategory));

enum tx_type_t
{
    none,
    begin,
    commit,
    rollback,
    max
};

struct rule_data_t
{
    string ruleset;
    string rule_name;
    string rule;
    string qualified_rule;
    string event_type;
    string gaia_type;
    string field;
    tx_type_t tx;
};

map<string, vector<rule_data_t>> g_rulesets;
string g_current_ruleset;
bool g_verbose = false;
unordered_map<string, string> g_includes;

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



class Rule_Subscriber_Visitor
  : public RecursiveASTVisitor<Rule_Subscriber_Visitor> 
{
public:

    explicit Rule_Subscriber_Visitor(ASTContext *context)
    {
        m_context = context;
    }

    // Find all the gaia types we care about.  Gaia types inherit
    // from gaia::direct_access::gaia_object_t.
    virtual bool VisitCXXRecordDecl(CXXRecordDecl *Declaration) 
    {
        if (!(Declaration->hasDefinition()))
        {
            return true;
        }

        for (auto base_it : Declaration->bases())
        {
            auto  baseDecl = base_it.getType()->getAsCXXRecordDecl();

            if (!baseDecl)
            {
                continue;
            }
            
            string baseQualifiedName = baseDecl->getQualifiedNameAsString();
            if (0 == baseQualifiedName.compare("gaia::direct_access::gaia_object_t"))
            {
                // We care about this object.  Add the name
                // and fully qualified name to map this symbol
                // to the include.
                string include_file;
                get_include_file(Declaration, include_file);
                g_includes.insert(
                    make_pair(Declaration->getNameAsString(), include_file));
                g_includes.insert(
                    make_pair(Declaration->getQualifiedNameAsString(),include_file));

                log("Found gaia type '%s'", Declaration->getQualifiedNameAsString());
                log(" in header '%s'\n", include_file);
                break;
            }
        }

        return true;
    }

    // Parse Comments for namespace.
    virtual bool VisitNamespaceDecl(clang::NamespaceDecl* d)
    {
        const ASTContext& context = d->getASTContext();
        const RawComment* rawComment = context.getRawCommentForDeclNoCache(d);
        
        if (rawComment)
        {
            const string prefix = "ruleset";
            string comment = rawComment->getBriefText(context);
            if (comment.compare(0, prefix.size(), prefix) == 0)
            {
                g_current_ruleset = d->getNameAsString();
                if (g_rulesets.find(g_current_ruleset) == g_rulesets.end())
                {
                    vector<rule_data_t> v;
                    g_rulesets.insert(make_pair(g_current_ruleset, std::move(v)));
                }
            }
            else
            {
                g_current_ruleset = "";
            }
        }
        else
        {
            g_current_ruleset = "";
        }
      
        return true;
    }

    // Parse comments for function declaration
    virtual bool VisitFunctionDecl(FunctionDecl *d)
    {
        /*
        const std::size_t rule_name_index = 1;
        const std::size_t gaia_type_index = 2;
        const std::size_t event_type_start_index = 3;
        const std::size_t minimal_valid_rule_annotation_size = 4;
        */
        const ASTContext& context = d->getASTContext();
        const RawComment* rawComment = context.getRawCommentForDeclNoCache(d);

        if (!rawComment)
        {
            return true;
        }

        const string prefix = "rule-";
        string comment = rawComment->getBriefText(context);
        // Check if first word of a comment is rule, if not it is not a rule annotation.
        if (comment.compare(0, prefix.size(), prefix) != 0)
        {
            return true;
        }

        if (!g_current_ruleset.empty())
        {
            auto& rules = g_rulesets[g_current_ruleset];
            rule_data_t rule_data;
            rule_data.ruleset = g_current_ruleset;
            rule_data.rule = d->getNameAsString();
            rule_data.qualified_rule = d->getQualifiedNameAsString();

            if (!parse_rule_name(comment, rule_data.rule_name))
            {
                llvm::errs() << "The rule configuration is invalid: Incorrect rule name\n";
                return true;
            }

            // Parse rule declarations.  A declaration is composed of a source
            // followed by a list of events.  A single function can be 
            // bound to multiple rule declarations.  Declaratiosn are separated
            // by semi-colons.
            vector<string> declarations;
            if (!parse_declarations(comment, declarations))
            {
                llvm::errs() << "The rule configuration is invalid: No declarations found.\n";
                return true;
            }

            for (auto decl : declarations)
            {
                if (!parse_source(decl, rule_data.gaia_type, rule_data.field))
                {
                    llvm::errs() << "The rule configuration is invalid: Invalid event source\n";
                    continue;
                }

                vector<string> events;
                string qualifier = "event_type_t::";

                if (!parse_events(decl, events))
                {
                    if (rule_data.field.empty())
                    {
                        llvm::errs() << "The rule configuration is invalid: Invalid event list\n";
                    }
                    else
                    {
                        rule_data.event_type = qualifier + "row_update";
                        rules.push_back(rule_data);
                    }
                    continue;
                }

                for (auto event : events)
                {
                    // Fully qualify and validate our events.
                    if (is_transaction_event(event, rule_data.tx))
                    {
                        if (!rule_data.gaia_type.empty() || !rule_data.field.empty())
                        {
                            llvm::errs() << "The rule configuration is invalid: Transaction events must not specify a source\n";
                            continue;
                        }
                        rule_data.event_type = qualifier + "transaction_" + event;
                    }
                    else
                    if (is_table_event(event))
                    {
                        if (rule_data.gaia_type.empty() || !rule_data.field.empty())
                        {
                            llvm::errs() << "The rule configuration is invalid: Table events must only specify a [gaia_type] source\n";
                            continue;
                        }
                        rule_data.event_type = qualifier + "row_" + event;
                    }
                    else
                    {
                        llvm::errs() << "The rule configuration is invalid: Invalid valid event type\n";
                        continue;
                    }
                    rules.push_back(rule_data);
                }
            }
        }
        else
        {
            llvm::errs() << "No ruleset was defined\n";
        }

        return true;
    }

private:

    void get_include_file(CXXRecordDecl * decl, string& filename)
    {
        FullSourceLoc loc = m_context->getFullLoc(decl->getBeginLoc());
        string full_path = loc.getFileEntry()->getName();
        size_t pos = full_path.find_last_of('/');
        if (pos == string::npos) 
        {
            filename = full_path;
        }
        else
        {
            pos++;
            filename = full_path.substr(pos, full_path.length() - pos);
        }
    }

    bool is_transaction_event(const string& event, tx_type_t& type)
    {
        type = tx_type_t::none;

        if (0 == event.compare("begin"))
        {
            type = tx_type_t::begin;
        }
        else
        if (0 == event.compare("rollback"))
        {
            type = tx_type_t::rollback;
        }
        else
        if (0 == event.compare("commit"))
        {
            type = tx_type_t::commit;
        }

        return (type != tx_type_t::none);
    }

    bool is_table_event(const string& event)
    {
        return (0 == event.compare("update"))
            || (0 == event.compare("insert"))
            || (0 == event.compare("delete"));
    }

    bool is_field_event(const string& event)
    {
        return (0 == event.compare("read"))
            || (0 == event.compare("write"));
    }

    bool parse_declarations(const string& text, vector<string>& declarations)
    {
        size_t start = text.find('[');
        if (start == string::npos)
        {
            return false;
        }
        string declarations_list = text.substr(start);
        split(declarations_list, declarations, ';');
        return true;
    }

    bool parse_rule_name(const string& text, string& name)
    {
        size_t end = text.find(':');
        if (end == string::npos) {
            return false;
        }
        name = text.substr(0, end);
        log("rule_name: %s\n", name);
        return true;
    }

    bool parse_source(const string& text, string& gaia_type, string& field)
    {
        gaia_type.clear();
        field.clear();

        size_t start = text.find('[');
        if (start == string::npos)
        {
            return false;
        }
        
        size_t end = text.find(']');
        if (end == string::npos)
        {
            return false;
        }
        
        start++;
        size_t dot = text.find('.');
        if (dot == string::npos)
        {
            gaia_type = text.substr(start, end - start);
        }
        else
        {
            gaia_type = text.substr(start, dot - start);
            ++dot;
            field = text.substr(dot, end - dot);
        }

        log("gaia_type: %s\n", gaia_type);
        log("field: %s\n", field);
        return true;
    }

    bool parse_events(const string& text, vector<string>& events)
    {
        size_t start = text.find('(');
        if (start == string::npos)
        {
            return false;
        }
        start++;

        size_t end = text.find(')', start);
        if (end == string::npos)
        {
            return false;
        }
        string event_list = text.substr(start, end-start);
        split(event_list, events, ',');
        return true;
    }

    ASTContext * m_context;
};

class Rule_Subscriber_Consumer : public clang::ASTConsumer 
{
public:
    explicit Rule_Subscriber_Consumer(ASTContext *context)
        : visitor(context) {}

    virtual void HandleTranslationUnit(clang::ASTContext &context) 
    {
        visitor.TraverseDecl(context.getTranslationUnitDecl());
    }
private:
    Rule_Subscriber_Visitor visitor;
};

class Rule_Subscriber_Action : public clang::ASTFrontendAction 
{
public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance &compiler, llvm::StringRef inFile) 
    {
        return std::unique_ptr<clang::ASTConsumer>(
            new Rule_Subscriber_Consumer(&compiler.getASTContext()));
    }
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

// Use our "none" descriptor to track whether we've created
// the gaia::rules namespace scope yet
void beginTxHookNamespace(stringstream& code)
{
    code << "namespace gaia" << endl;
    code << "{" << endl;
    code << "namespace rules" << endl;
    code << "{" << endl;
}

void endTxHookNamespace(stringstream& code)
{
    code << "}" << endl;
    code << "}" << endl;
}

void generateTxHookFunctions(stringstream& code, const map<string, vector<rule_data_t>>& rulesets)
{
    // Always generate the commit and rollback hooks but the hook implementation
    // will differ.
    // Note that binding to a begin transcation event is ignored for now.
    bool gen_commit_event = false;

    for (auto it_rulesets = rulesets.cbegin(); it_rulesets != rulesets.cend(); ++it_rulesets)
    {
        auto rules = it_rulesets->second;
        // Generate rules forward declarations and gather includes
        for (auto it_rules = rules.cbegin(); it_rules != rules.cend(); ++it_rules)
        {
            if (it_rules->tx == tx_type_t::commit)
            {
                gen_commit_event = true;
                break;
            }
        }
    }

    beginTxHookNamespace(code);

    // Always generate rollback hook (which implicitly fires event_type_t::transaction_rollback)
    code << "static void rollback_tx_hook()" << endl;
    code << "{" << endl;
    code << "    rollback_trigger();" << endl;
    code << "    gaia_base_t::rollback_hook();" << endl;    
    code << "}" << endl;

    // Always generate commit hook
    code << "static void commit_tx_hook()" << endl;
    code << "{" << endl;
    if (gen_commit_event)
    {
        code << "    trigger_event_t event = {event_type_t::transaction_commit, 0, 0, nullptr, 0};" << endl;
        code << "    commit_trigger(0, &event, 1, true);" << endl;
    }
    else
    {
        code << "    commit_trigger(0, nullptr, 0, true);" << endl;
    }
    code << "    gaia_base_t::commit_hook();" << endl;    
    code << "}" << endl;
    
    endTxHookNamespace(code);
    code << endl;
}

void generateTxHookInit(stringstream& code)
{
    // We always have hooks for commit and rollback but begin is optional.
    code << "    gaia::db::s_tx_commit_hook = gaia::rules::commit_tx_hook;" << endl;
    code << "    gaia::db::s_tx_rollback_hook = gaia::rules::rollback_tx_hook;" << endl;
}

void generate_ruleset_namespace(stringstream& code, const string& ruleset, const vector<rule_data_t>& rules)
{
    unordered_set<std::string> declarations;

    code << "namespace " << ruleset << " {" << endl;
    for (auto it = rules.cbegin(); it != rules.cend(); ++it)
    {
        if (declarations.find(it->rule) == declarations.end()) {
            code << "   void " << it->rule << "(const rule_context_t* context);" << endl;
            declarations.insert(it->rule);
        }
    }
    code << "}" << endl;
}

void generate_ruleset_subunsub(stringstream& code, const string& ruleset, const vector<rule_data_t>& rules, bool subscribe)
{
    unordered_set<std::string> declarations;
    string action = subscribe ? "subscribe_" : "unsubscribe_";

    code << "void " << action << ruleset << "()" << endl;
    code << "{" << endl;

    // Generate rule binding structures.
    for (auto it = rules.cbegin(); it != rules.cend(); ++it)
    {
        declarations.emplace("    rule_binding_t  " + it->rule + "(\"" 
            + it->ruleset +  "\",\"" + it->rule_name + "\"," + it->qualified_rule + ");");
    }

    for (auto it = declarations.cbegin(); it != declarations.cend(); ++it)
    {
        code << *it << endl;
    }
    code << endl;
    
    // Generate subscription code.
    uint32_t field_list_decl = 1;
    for (auto it = rules.cbegin(); it != rules.cend(); ++it)
    {
        // If not type is given (transaction events, for example) then
        // Just use 0 as the gaia type.  Otherwise, use the static id built
        // off the member.
        string gaia_type = it->gaia_type.empty() ? "0" : 
            (it->gaia_type + "::s_gaia_type");


        string field_list = "fields_" + to_string(field_list_decl);
        if (!(it->field.empty()))
        {
            code << "    field_list_t " << field_list << ";" << endl;
            code << "    " << field_list << ".insert(" << it->gaia_type << "::get_field_offset(\"" << it->field << "\"));" << endl;
            code << "    " << action << "rule(" << gaia_type << ", "  << it->event_type << ", " << field_list << ", " << it->rule << ");" << endl;
            field_list_decl++;
        }
        else
        {
            code << "    " << action << "rule(" << gaia_type << ", "  << it->event_type << ", gaia::rules::empty_fields, " << it->rule << ");" << endl;
        }
        
        code << endl;
    }

    code << "}" << endl;
}

void generate_ruleset_dispatcher(stringstream& code, const map<string, vector<rule_data_t>>& rulesets, bool subscribe) 
{
    string action = subscribe ? "subscribe_" : "unsubscribe_";
    // generate functions for subscribing, unsubscribing based on ruleset name
    code << "extern \"C\" void " << action << "ruleset(const char* ruleset_name)" << endl;
    code << "{" << endl;

    for (auto it_rulesets = rulesets.cbegin(); it_rulesets != rulesets.cend(); ++it_rulesets)
    {
        code << "   if (strcmp(ruleset_name, \"" << it_rulesets->first << "\") == 0)" << endl;
        code << "   {" << endl;
        code << "       " << action << it_rulesets->first << "();" << endl;
        code << "       return;" << endl;
        code << "   }" << endl;
    }

    code << "   throw ruleset_not_found(ruleset_name);" << endl;
    code << "}" << endl;
}

void generateCode(const char *fileName, const map<string, vector<rule_data_t>>& rulesets)
{
    unordered_set<string> declarations;
    unordered_set<string> includes;

    for (auto it_rulesets = rulesets.cbegin(); it_rulesets != rulesets.cend(); ++it_rulesets)
    {
        auto rules = it_rulesets->second;
        // Generate rules forward declarations and gather includes
        for (auto it_rules = rules.cbegin(); it_rules != rules.cend(); ++it_rules)
        {
            declarations.emplace(it_rules->qualified_rule);
            if (!it_rules->gaia_type.empty())
            {
                auto inc_it = g_includes.find(it_rules->gaia_type);
                if (inc_it != g_includes.end())
                {
                    includes.insert(inc_it->second);
                }
                else
                {
                    log("Didn't find header for gaia_type '%s'\n", it_rules->gaia_type);
                }
            }
        }
    }

    // Generate includes.
    stringstream code;
    code << "#include \"rules.hpp\"" << endl;
    for (auto include : includes)
    {
        code << "#include \"" << include << "\"" << endl;
    }
    code << endl;

    code << "using namespace gaia::rules;" << endl; 
    code << endl;

    generateTxHookFunctions(code, rulesets);

    // generate ruleset init functions
    for (auto it_rulesets = rulesets.cbegin(); it_rulesets != rulesets.cend(); ++it_rulesets)
    {
        generate_ruleset_namespace(code, it_rulesets->first, it_rulesets->second);
        generate_ruleset_subunsub(code, it_rulesets->first, it_rulesets->second, true /*subscribe*/);
        generate_ruleset_subunsub(code, it_rulesets->first, it_rulesets->second, false /*unsubscribe*/);
    }

    // generate ruleset dispatch functions
    generate_ruleset_dispatcher(code, rulesets, true /*subscribe*/);
    generate_ruleset_dispatcher(code, rulesets, false /*unsubscribe*/);


    // generate initialize_rules functions
    // generate subscribe/unsubscribe 
    // generate overloads that just take a string for the ruleset name
    code << "extern \"C\" void initialize_rules()" << endl;
    code << "{" << endl;

    // generate calls to each generated initialize_ruleset function
    for (auto it_rulesets = rulesets.cbegin(); it_rulesets != rulesets.cend(); ++it_rulesets)
    {
        code << "    subscribe_" << it_rulesets->first << "();" << endl;
    }

    // Setup transaction hooks if needed.
    generateTxHookInit(code);
    code << "}" << endl;

    SaveFile(fileName, code);
}


int main(int argc, const char **argv) 
{

    // Parse the command-line args passed to your code.
    CommonOptionsParser op(argc, argv, RuleSubscriberCategory);
    if (RuleSubscriberVerboseOption)
    {
        g_verbose = true;
    }
    // Create a new Clang Tool instance (a LibTooling environment).
    ClangTool tool(op.getCompilations(), op.getSourcePathList());
    tool.run(newFrontendActionFactory<Rule_Subscriber_Action>().get());
    generateCode(RuleSubscriberOutputOption.c_str(), g_rulesets);

}