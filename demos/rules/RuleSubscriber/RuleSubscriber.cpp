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
cl::opt<string> RuleSubscriberOption("output",cl::init(""), cl::desc("output file name"), cl::cat(RuleSubscriberCategory));


struct rule_data
{
    string ruleset;
    string rulename;
    string rule;
    string eventType;
    string gaiaType;
    bool isTransactionRule;
};

vector<rule_data> rules;
string currentRuleset;

class RuleSubscriberVisitor
  : public RecursiveASTVisitor<RuleSubscriberVisitor> 
{
public:
  explicit RuleSubscriberVisitor(ASTContext *Context)
    : Context(Context) {}

    virtual bool VisitNamespaceDecl(clang::NamespaceDecl* d)
    {
      ASTContext& ctx = d->getASTContext();
      const RawComment* rc = ctx.getRawCommentForDeclNoCache(d);
      if (rc)
      {
        const string prefix = "ruleset";
        string comment = rc->getBriefText(ctx);
        if (comment.compare(0, prefix.size(), prefix) == 0)
        {
          currentRuleset = d->getNameAsString();
        }
        else
        {
         currentRuleset = "";
        }
      }
      else
      {
        currentRuleset = "";
      }
      
      return true;
    }

    virtual bool VisitFunctionDecl(FunctionDecl *d)
    {
      ASTContext& ctx = d->getASTContext();
      const RawComment* rc = ctx.getRawCommentForDeclNoCache(d);
      if (rc)
      {
        const string prefix = "rule";
        string comment = rc->getBriefText(ctx);
        if (comment.compare(0, prefix.size(), prefix) == 0)
        {
          if (!currentRuleset.empty())
          {
            vector<string> params = split(comment, ',');
            if (params.size() >= 4 )
            {
              string ruleName = trim(params[1]);
              string gaiaType = trim(params[2]);
              if (!ruleName.empty())
              {
                for (int idx = 3; idx < params.size(); idx++)
                {
                  rule_data ruleData;
                  string eventType = params[idx];
                  ruleData.ruleset = currentRuleset;
                  ruleData.rulename = trim(params[1]);
                  ruleData.isTransactionRule = eventType.find("transaction_") != string::npos;
                  ruleData.gaiaType = gaiaType;
                  ruleData.rule = d->getQualifiedNameAsString();

                  if (!eventType.empty() && !(!ruleData.isTransactionRule && gaiaType.empty()) )
                  {
                      ruleData.eventType = eventType;
                      rules.push_back(ruleData);
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
        }
      }
      return true;
    }


    

private:

  vector<string> split(const string &text, char sep) 
  {
    vector<string> tokens;
    size_t start = 0, end = 0;
    while ((end = text.find(sep, start)) != string::npos) 
    {      
        tokens.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(text.substr(start));
    return tokens;
  }

  string &ltrim(string &s) 
  {
    s.erase(s.begin(), find_if(s.begin(), s.end(),
            not1(ptr_fun<int, int>(isspace))));
    return s;
  }

// trim from end
string &rtrim(string &s) 
{
    s.erase(find_if(s.rbegin(), s.rend(),
            not1(ptr_fun<int, int>(isspace))).base(), s.end());
    return s;
}

// trim from both ends
string &trim(string &s) {
    return ltrim(rtrim(s));
}

  ASTContext *Context;
};
/*class MyDiagnosticConsumer : public clang::DiagnosticConsumer {
public:
  void HandleDiagnostic(clang::DiagnosticsEngine::Level DiagLevel,
                        const clang::Diagnostic& Info) override {
    llvm::SmallVector<char, 128> message;
    Info.FormatDiagnostic(message);
    llvm::errs() << DiagLevel << "  " << message << '\n';
  }
};
*/
class RuleSubscriberConsumer : public clang::ASTConsumer 
{
public:
  explicit RuleSubscriberConsumer(ASTContext *Context)
    : Visitor(Context) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) 
  {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }
private:
  RuleSubscriberVisitor Visitor;
};

class RuleSubscriberAction : public clang::ASTFrontendAction 
{
public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) 
  {
//    auto& DE = Compiler.getASTContext().getDiagnostics();
//    DE.setClient(new MyDiagnosticConsumer(), /*ShouldOwnClient=*/true);
    return std::unique_ptr<clang::ASTConsumer>(
        new RuleSubscriberConsumer(&Compiler.getASTContext()));
  }
};

bool SaveFile(const char *name, const stringstream & buf) 
{
  std::ofstream ofs(name, std::ofstream::out);
  if (!ofs.is_open()) return false;
  ofs << buf.str();
  return !ofs.bad();
}

void generateCode(const char * fileName, const vector<rule_data>& rules)
{
    unordered_set<string> declarations;
    
    stringstream code;
    code <<"#include \"rules.hpp\"" << endl;
    code << "using namespace gaia::rules;" << endl; 
    
    
    //generate rules forward declarations
    for (vector<rule_data>::const_iterator it = rules.cbegin();
             it != rules.cend();
             ++it)
    {
      declarations.emplace( "void " + it->rule + "(const context_base_t * context);");        
    }
    for (unordered_set<string>::const_iterator it = declarations.cbegin();
      it != declarations.cend();
      ++it)
    {
      code << *it << endl;   
    }

    declarations.clear();

    code << "gaia::common::error_code_t gaia_init_rules()" <<endl;
    code << "{" << endl;
    //generate rule binding structure
    for (vector<rule_data>::const_iterator it = rules.cbegin();
             it != rules.cend();
             ++it)
    {
      declarations.emplace("    rule_binding_t  " + it->ruleset + "_" + it->rule + "(\"" + it->ruleset +  "\",\"" + it->rulename + "\"," + it->rule + ");");        
    }

    for (unordered_set<string>::const_iterator it = declarations.cbegin();
      it != declarations.cend();
      ++it)
    {
      code << *it << endl;   
    }
    
    code << "    gaia::common::error_code_t retVal;" << endl;

    //generate subscription code
    for (vector<rule_data>::const_iterator it = rules.cbegin();
             it != rules.cend();
             ++it)
    {
        if (it->isTransactionRule)
        {
            code << "    retVal = subscribe_transaction_rule(" << it->eventType << "," << it->ruleset << "_" << it->rule << ");" << endl;
        }
        else
        {
            code << "    retVal = subscribe_table_rule(" << it->gaiaType << "," << it->eventType << "," << it->ruleset << "_" << it->rule << ");" << endl;
        }

        code << "    if (retVal != error_code_t::success) return retVal;" << endl;
    }

    code << "    return retVal;" << endl;
    code << "}";
    SaveFile(fileName, code);
}


int main(int argc, const char **argv) 
{

  // parse the command-line args passed to your code
  CommonOptionsParser op(argc, argv, RuleSubscriberCategory);        
  // create a new Clang Tool instance (a LibTooling environment)
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());
  Tool.run(newFrontendActionFactory<RuleSubscriberAction>().get());
  generateCode(RuleSubscriberOption.c_str(), rules);  
}