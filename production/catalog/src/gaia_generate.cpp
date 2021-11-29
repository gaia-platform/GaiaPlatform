/////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"

#include "dac_generator.hpp"

namespace gaia
{
namespace catalog
{

gaia_database_t find_database(const std::string& db_name)
{
    auto database = gaia_database_t::list().where(gaia_database_expr::name == db_name);

    if (database.begin() == database.end())
    {
        throw db_does_not_exist(db_name);
    }

    return *database.begin();
}

std::string generate_dac_header(const std::string& db_name)
{
    db::begin_transaction();
    generate::dac_compilation_unit_writer_t file_writer{find_database(db_name)};
    std::string header_content = file_writer.write_header();
    db::commit_transaction();
    return header_content;
}

std::string generate_dac_cpp(const std::string& db_name, const std::string& header_file_name)
{
    db::begin_transaction();
    generate::dac_compilation_unit_writer_t file_writer{find_database(db_name)};
    std::string cpp_content = file_writer.write_cpp();
    cpp_content.replace(cpp_content.find("{GENERATED_DAC_HEADER}"), strlen("{GENERATED_DAC_HEADER}"), header_file_name);
    db::commit_transaction();
    return cpp_content;
}

} // namespace catalog
} // namespace gaia
