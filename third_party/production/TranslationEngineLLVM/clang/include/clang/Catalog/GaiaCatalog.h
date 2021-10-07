/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

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
    unordered_map<string, CatalogFieldData> fieldData;
    unordered_map<string, CatalogLinkData> linkData;
    string dbName;
};

class GaiaCatalog
{
public:
    static const unordered_map<string, CatalogTableData>& getCatalogTableData()
    {
        ensureInitialization();
        return catalogTableData;
    }

private:
    static void ensureInitialization();
    static void fillTableData();
    static bool isInitialized;
    static unordered_map<string, CatalogTableData> catalogTableData;
};
} // namespace catalog
} // namespace gaia
} // namespace clang
