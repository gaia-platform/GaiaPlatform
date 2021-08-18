/////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////

#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#include "clang/Basic/Diagnostic.h"
#pragma clang diagnostic pop

namespace gaia
{
namespace translation
{

// Strings used to build the version string
static const char c_gaiat[] = "Gaia Translation Engine";
static const char c_copyright[] = "Copyright (c) Gaia Platform LLC";

// This is a command line error that can occur before we even get the
// llvm diagnostic sub-system.
static const char c_err_multiple_ruleset_files[] = 
    "The Translation Engine does not support more than one source ruleset file. "
    "Combine your rulesets into a single ruleset file.\n";

// Helper class to emit llvm diagnostics.
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
