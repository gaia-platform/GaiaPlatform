#include "clang/Driver/Options.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_set>
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

struct rule_data_t
{
    string ruleset;
    string rule_name;
    string rule;
    string event_type;
    string gaia_type;
    string field;
};

vector<rule_data_t> g_rules;
string g_current_ruleset;
bool g_verbose = false;

class Rule_Subscriber_Visitor
  : public RecursiveASTVisitor<Rule_Subscriber_Visitor> 
{
public:
    explicit Rule_Subscriber_Visitor(ASTContext *context)
    {

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
            rule_data_t rule_data;
            rule_data.ruleset = g_current_ruleset;
            rule_data.rule = d->getQualifiedNameAsString();

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
                if (!parse_events(decl, events))
                {
                    llvm::errs() << "The rule configuration is invalid: Invalid event list\n";
                    continue;
                }
                for (auto event : events)
                {
                    string qualifier = "event_type_t::";

                    // Fully qualify and validate our events.
                    if (is_transaction_event(event))
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
                    if (is_field_event(event))
                    {
                        if (rule_data.gaia_type.empty() || rule_data.field.empty())
                        {
                            llvm::errs() << "The rule configuration is invalid: Field events must specify a qualified "
                                "[gaia_type.field] source\n";
                            continue;
                        }
                        rule_data.event_type = qualifier + "field_" + event;
                    }
                    else
                    {
                        llvm::errs() << "The rule configuration is invalid: Invalid valid event type\n";
                        continue;
                    }
                    g_rules.push_back(rule_data);
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
    void log(const char* message, std::string& value)
    {
        if (g_verbose)
        {
            printf(message, value.c_str());
        }
    }

    bool is_transaction_event(const string& event)
    {
        return (0 == event.compare("begin"))
            || (0 == event.compare("rollback"))
            || (0 == event.compare("commit"));
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

void generateCode(const char *fileName, const vector<rule_data_t>& rules)
{
    unordered_set<string> declarations;
    
    stringstream code;
    code << "#include \"rules.hpp\"" << endl;
    code << "using namespace gaia::rules;" << endl; 
    
    //Generate rules forward declarations.
    for (auto it = rules.cbegin(); it != rules.cend(); ++it)
    {
      declarations.emplace( "void " + it->rule + "(const rule_context_t *context);");        
    }

    for (auto it = declarations.cbegin(); it != declarations.cend(); ++it)
    {
        code << *it << endl;   
    }

    declarations.clear();

    code << "extern \"C\" void initialize_rules()" << endl;
    code << "{" << endl;
    // Generate rule binding structure.
    for (auto it = rules.cbegin(); it != rules.cend(); ++it)
    {
        declarations.emplace("    rule_binding_t  " + it->rule + "(\"" 
            + it->ruleset +  "\",\"" + it->rule_name + "\"," + it->rule + ");");
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
        // Just use 0 as the gaia type.  Otherwise, call type gaia_type_id()
        // method to get this at runtime.
        string gaia_type = it->gaia_type.empty() ? "0" : 
            (it->gaia_type + "::gaia_type_id()");

        if (it->field.empty())
        {
            code << "    subscribe_database_rule(" << gaia_type << ", " 
                << it->event_type << ", " << it->rule << ");" << endl;
        }
        else
        {
            string field_list = "fields_" + to_string(field_list_decl);
            code << "    field_list_t " << field_list << ";" << endl;
            code << "    " << field_list << ".insert(\"" << it->field << "\");" << endl;
            code << "    subscribe_field_rule(" << gaia_type << ", " 
                << it->event_type << ", " << field_list << ", " << it->rule << ");" << endl;
            field_list_decl++;
        }

        code << endl;
    }

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
    generateCode(RuleSubscriberOutputOption.c_str(), g_rules);

}