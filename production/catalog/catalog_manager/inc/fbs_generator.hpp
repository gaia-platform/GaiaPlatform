/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "gaia_catalog.hpp"
#include "catalog_gaia_generated.h"

namespace gaia {
namespace catalog {

namespace ddl {

class unknown_data_type : public gaia::common::gaia_exception {
  public:
    unknown_data_type();
};

} // namespace ddl

/**
 * Generate FlatBuffers schema (fbs) from parsing result of a table.
 * Before the support of complex types, we can generate a table schema from its own own definition.
 * This method has no dependency on catalog or other table definitions.
 * Note: If we begin to supoort complex types, the method will need to be updated sigificantly.
 *
 * @param table_name table name
 * @param fields table fields parsing result bindings
 * @return fbs schema
 */
string generate_fbs(const string &table_name, const ddl::field_def_list_t &fields);

/**
 * Generate binary FlatBuffers schema (bfbs) in base64 encoded string format.
 *
 * @param valid FlatBuffers schema
 * @return base64 encoded bfbs string
 */
string generate_bfbs(const string &fbs);

/**
 * Get the data type name (in string)
 *
 * @param catalog data type
 * @return data type name in string
 * @throw unknown_data_type
 */
string get_data_type_name(data_type_t data_type);

} // namespace catalog
} // namespace gaia
