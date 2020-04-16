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

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;

cl::OptionCategory RuleSubscriberCategory("Use Rule Subscriber options");
cl::opt<string> RuleSubscriberOption("output", cl::init(""), 
    cl::desc("output file name"), cl::cat(RuleSubscriberCategory));

struct rule_data
{
    string ruleset;
    string rule_name;
    string rule;
    string qualified_rule;
    string event_type;
    string gaia_type;
    bool is_transaction_rule;
};

vector<rule_data> rules;

vector<string> split(const string &text, char separator) 
    {
        vector<string> tokens;
        size_t start = 0, end = 0;
        
        while ((end = text.find(separator, start)) != string::npos) 
        {      
            tokens.push_back(text.substr(start, end - start));
            start = end + 1;
        }
        
        tokens.push_back(text.substr(start));
        return tokens;
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
string current_ruleset;



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
                current_ruleset = d->getNameAsString();
            }
            else
            {
                current_ruleset = "";
            }
        }
        else
        {
            current_ruleset = "";
        }
      
        return true;
    }

    // Parse comments for function declaration
    virtual bool VisitFunctionDecl(FunctionDecl *d)
    {
        const std::size_t rule_name_index = 1;
        const std::size_t gaia_type_index = 2;
        const std::size_t event_type_start_index = 3;
        const std::size_t minimal_valid_rule_annotation_size = 4;
        const ASTContext& context = d->getASTContext();
        const RawComment* rawComment = context.getRawCommentForDeclNoCache(d);
        
        if (!rawComment)
        {
            return true;
        }
        
        const string prefix = "rule";
        string comment = rawComment->getBriefText(context);
        // Check if first word of a comment is rule, if not it is not a rule annotation.
        if (comment.compare(0, prefix.size(), prefix) != 0)
        {
            return true;
        }
        
        if (!current_ruleset.empty())
        {
            // Split the comments into words to get rule subscription parameters
            vector<string> params = split(comment, ',');
            if (params.size() >= minimal_valid_rule_annotation_size )
            {
                string rule_name = trim(params[rule_name_index]);
                string gaia_type = trim(params[gaia_type_index]);
                if (!rule_name.empty())
                {
                    for (std::size_t i = event_type_start_index; i < params.size(); i++)
                    {
                        rule_data ruleData;
                        string event_type = params[i];
                        ruleData.ruleset = current_ruleset;
                        ruleData.rule_name = rule_name;
                        ruleData.is_transaction_rule = event_type.find("transaction_") != string::npos;
                        ruleData.gaia_type = gaia_type;
                        ruleData.rule = d->getNameAsString();
                        ruleData.qualified_rule = d->getQualifiedNameAsString();
                        

                        // Check if event_type is valid i.e.
                        //  1. event_type is not empty and 
                        //  2. if rule is table rule then gaia_type is not empty      
                        if (!event_type.empty() && !(!ruleData.is_transaction_rule && gaia_type.empty()))
                        {
                            if (ruleData.is_transaction_rule || !gaia_type.empty())
                            {
                                ruleData.event_type = event_type;
                                rules.push_back(ruleData);
                            }
                            else
                            {
                                llvm::errs() << "The rule configuration is invalid: Empty gaia type for table event\n";
                            }                           
                        }
                        else
                        {
                            llvm::errs() << "The rule configuration is invalid: Incorrect event type\n";
                        }
                    }
                }
                else
                {
                    llvm::errs() << "The rule configuration is invalid: Incorrect rule name\n";
                }             
            }
            else
            {
                llvm::errs() << "The rule configuration is invalid: Not enough parameters\n";
            }
        }
        else
        {
            llvm::errs() << "No ruleset was defined\n";
        }

        return true;
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

void generateCode(const char *fileName, const vector<rule_data>& rules)
{
    unordered_set<string> declarations;
    
    stringstream code;
    code << "#include \"rules.hpp\"" << endl;
    code << "using namespace gaia::rules;" << endl; 
    
    //Generate rules forward declarations.
    for (auto it = rules.cbegin(); it != rules.cend(); ++it)
    {
      declarations.emplace(it->qualified_rule);        
    }

    for (auto it = declarations.cbegin(); it != declarations.cend(); ++it)
    {
        vector<string> namespaces = split(*it,':');
        int namespace_count = 0;
        for (auto namespace_iterator = namespaces.cbegin(); namespace_iterator != namespaces.cend() - 1; ++namespace_iterator)
        {
            if (!namespace_iterator->empty())
            {
                code << "namespace " << *namespace_iterator << "{" << endl;
                namespace_count ++;
            }
        }
        code << "void " << *(namespaces.cend() - 1) << "(const context_base_t *context);" << endl;
        for (int namespace_count_idx = 0; namespace_count_idx < namespace_count; ++ namespace_count_idx)
        {
            code << "}" << endl;
        }   
    }

    declarations.clear();

    for (auto it = rules.cbegin(); it != rules.cend(); ++it)
    {
        if (!it->is_transaction_rule)
        {
            declarations.emplace( it->gaia_type);  
        }      
    }

    for (auto it = declarations.cbegin(); it != declarations.cend(); ++it)
    {
        vector<string> namespaces = split(*it,':');
        int namespace_count = 0;
        for (auto namespace_iterator = namespaces.cbegin(); namespace_iterator != namespaces.cend() - 1; ++namespace_iterator)
        {
            if (!namespace_iterator->empty())
            {
                code << "namespace " << *namespace_iterator << "{" << endl;
                namespace_count ++;
            }
        }
        code << "gaia::common::gaia_type_t " << *(namespaces.cend() - 1) << ";" << endl;
        for (int namespace_count_idx = 0; namespace_count_idx < namespace_count; ++ namespace_count_idx)
        {
            code << "}" << endl;
        }
    }

    declarations.clear();

    code << "extern \"C\" void initialize_rules()" << endl;
    code << "{" << endl;
    // Generate rule binding structure.
    for (auto it = rules.cbegin(); it != rules.cend(); ++it)
    {
        declarations.emplace("    rule_binding_t  " + it->rule + "(\"" 
            + it->ruleset +  "\",\"" + it->rule_name + "\"," + it->qualified_rule + ");");        
    }

    for (auto it = declarations.cbegin(); it != declarations.cend(); ++it)
    {
        code << *it << endl;   
    }
    
    // Generate subscription code.
    for (auto it = rules.cbegin(); it != rules.cend(); ++it)
    {
        if (it->is_transaction_rule)
        {
            code << "    subscribe_transaction_rule(" << it->event_type << "," 
                << it->rule << ");" << endl;
        }
        else
        {
            code << "    subscribe_table_rule(" << it->gaia_type << "," 
                << it->event_type << "," << it->rule << ");" << endl;
        }

    }

    code << "}";

    SaveFile(fileName, code);
}


int main(int argc, const char **argv) 
{

    // Parse the command-line args passed to your code.
    CommonOptionsParser op(argc, argv, RuleSubscriberCategory);        
    // Create a new Clang Tool instance (a LibTooling environment).
    ClangTool tool(op.getCompilations(), op.getSourcePathList());
    tool.run(newFrontendActionFactory<Rule_Subscriber_Action>().get());
    generateCode(RuleSubscriberOption.c_str(), rules);  

}