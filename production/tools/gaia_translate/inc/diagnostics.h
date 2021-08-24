/////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////

#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#include "clang/Basic/Diagnostic.h"
#pragma clang diagnostic pop

// When adding errors strings to the translation engine, please use the LLVM
// diagnostics infrastructure.  To add an error:
//
// 1) Check whether an appropriate error string already exists in DiagnosticsSemaKinds.td or DiagnosticParseKinds.td.
// Our errors are at the bottom of the file grouped by category (Error, Note, Warning), and then alphabetically.  Note
// whether the diagnostic has argument placeholders in the string (%0, %1). If so, be sure to provide values for these
// placeholders when reporting the diagnostic. If no suitable diagnostic exists, feel free to add it.
//
// 2) Use 'gaiat::diag().emit(...)' to get the diagnostics engine and pass in the error string.  The IDs and other info
// are generated from the DiagnosticsSemaKinds.td file and put into the 'diag' namespace.
//
// 3) You can set the source location to output the line and column number near where the diagnostic occurred.
// The line and column number information is for the .ruleset file and not the generated .cpp file.  Source
// location can be set on the global diag() object via the `diag().set_location(...)` method.  In addition, you can 
// pass a location as an argument to the emit() method itself.  A typical pattern is to set the source location on
// the diag() object at a higher level function and then let lower level functions pass in more specific locations
// as more context is gathered.
//
// As an example, to report "table 'foo' not found" you would write:
// gaiat::emit(diag::err_table_not_found) << table_name;
//
// To provide source location, you could write:
// gaiat::emit(my_source_location, diag::err_table_not_found) << table_name;
//
// Note that the arguments for placeholders in the diagnostic string (%0, %1, ...) are provided from left to right:
// gaiat::emit(err_two_params) << param_1 << param_2;
//

namespace gaia
{
namespace translation
{

// Strings used to build the version string
static const char c_gaiat[] = "Gaia Translation Engine";
static const char c_copyright[] = "Copyright (c) Gaia Platform LLC";

// This is a command line error that can occur before we even get the
// LLVM diagnostic sub-system.
static const char c_err_multiple_ruleset_files[] = 
    "The Translation Engine does not support more than one source ruleset file. "
    "Combine your rulesets into a single ruleset file.\n";

// Helper class to emit LLVM diagnostics.
class diagnostic_context_t
{
public:
    diagnostic_context_t() = delete;
    diagnostic_context_t(clang::DiagnosticsEngine& diag_engine)
    : m_diag_engine(diag_engine)
    {
    }

    clang::DiagnosticBuilder emit(unsigned diag_id)
    {
        return m_diag_engine.Report(m_loc, diag_id);
    }

    // Passed in SourceLocation will override any stored SourceLocation.
    clang::DiagnosticBuilder emit(clang::SourceLocation loc, unsigned diag_id)
    {
        return m_diag_engine.Report(loc, diag_id);
    }

    // Useful for setting up the context so that subsequent
    // calls to emit can use the stored location.
    void set_location(clang::SourceLocation loc)
    {
        m_loc = loc;
    }

private:
    clang::DiagnosticsEngine& m_diag_engine;
    clang::SourceLocation m_loc;
};

// Can be referenced using gaiat::diag()
diagnostic_context_t& diag();
// The gaiat diagnostic context is setup when we create the ASTConsumer.
extern std::unique_ptr<diagnostic_context_t> g_diag_ptr;

} // namespace translation
} // namespace gaia

namespace gaiat = gaia::translation;
