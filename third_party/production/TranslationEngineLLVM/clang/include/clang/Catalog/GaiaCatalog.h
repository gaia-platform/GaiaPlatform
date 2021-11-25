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
    static const llvm::StringMap<CatalogTableData>& getCatalogTableData()
    {
        ensureInitialization();
        return catalogTableData;
    }
    static bool findNavigationPath(llvm::StringRef src, llvm::StringRef dst, llvm::SmallVector<string, 8>& currentPath);

private:
    static void ensureInitialization();
    static void fillTableData();
    static bool findNavigationPath(llvm::StringRef src, llvm::StringRef dst, llvm::SmallVector<string, 8>& currentPath, const llvm::StringMap<clang::gaia::catalog::CatalogTableData>& graphData);
    static llvm::StringRef getClosestTable(const llvm::StringMap<int>& tableDistance);
    static bool isInitialized;
    static llvm::StringMap<CatalogTableData> catalogTableData;
};
} // namespace catalog
} // namespace gaia
} // namespace clang
