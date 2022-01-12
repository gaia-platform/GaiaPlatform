/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include "llvm/ADT/StringMap.h"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/catalog/catalog.hpp"

using namespace gaia;
using namespace gaia::common;
using namespace std;

namespace clang
{

namespace gaia
{

namespace catalog
{

struct CatalogFieldData
{
    bool isDeprecated;
    bool isActive;
    field_position_t position;
    data_type_t fieldType;
};

struct CatalogLinkData
{
    string targetTable;
    ::gaia::catalog::relationship_cardinality_t cardinality;
    bool isFromParent;
};

struct CatalogTableData
{
    llvm::StringMap<CatalogFieldData> fieldData;
    llvm::StringMap<CatalogLinkData> linkData;
    string dbName;
};

class GaiaCatalog
{
public:
    GaiaCatalog() = delete;
    GaiaCatalog(clang::DiagnosticsEngine& diag_engine);
    const llvm::StringMap<CatalogTableData>& getCatalogTableData();
    bool findNavigationPath(llvm::StringRef src, llvm::StringRef dst, llvm::SmallVector<string, 8>& currentPath, bool reportErrors);

private:
    void ensureInitialization();
    void fillTableData();
    bool findNavigationPath(llvm::StringRef src, llvm::StringRef dst, llvm::SmallVector<string, 8>& currentPath, const llvm::StringMap<clang::gaia::catalog::CatalogTableData>& graphData);
    llvm::StringRef getClosestTable(const llvm::StringMap<int>& tableDistance);

    bool m_isInitialized;
    llvm::StringMap<CatalogTableData> m_catalogTableData;
    DiagnosticsEngine& m_diags;
};

// Convenience wrappers to get the catalog instance and then access the catalog data.
void createCatalogInstance(clang::DiagnosticsEngine& diags);
const llvm::StringMap<CatalogTableData>& getCatalogTableData();
bool findNavigationPath(llvm::StringRef src, llvm::StringRef dst, llvm::SmallVector<string, 8>& currentPath, bool reportErrors = true);

} // namespace catalog
} // namespace gaia
} // namespace clang
