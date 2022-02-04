/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include "llvm/ADT/StringMap.h"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaiat_catalog_facade.hpp"

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
    bool isArray;
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
    static void create(clang::DiagnosticsEngine& diag);
    static GaiaCatalog& get();

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

private:
    static std::unique_ptr<GaiaCatalog> s_catalog_ptr;

private:
    bool m_isInitialized;
    llvm::StringMap<CatalogTableData> m_catalogTableData;
    DiagnosticsEngine& m_diags;
};

const llvm::StringMap<CatalogTableData>& getCatalogTableData();
bool findNavigationPath(llvm::StringRef src, llvm::StringRef dst, llvm::SmallVector<string, 8>& currentPath, bool reportErrors = true);

} // namespace catalog
} // namespace gaia
} // namespace clang
