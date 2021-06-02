/////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////

#include <fstream>

#include <spdlog/fmt/fmt.h>

#include <gaia_internal/catalog/gaia_catalog.h>

#include "code_generator.hpp"
#include "gaia_generate.hpp"

namespace gaia
{
namespace catalog
{
namespace generate
{

generation_error::generation_error(const std::string& message)
{
    m_message = message;
}

void gaia_edc_generator_t::generate(std::filesystem::path output_dir, std::string base_file_name)
{
    auto database = gaia_database_t::list().where(gaia_database_expr::name == m_database_name);

    if (database.begin() == database.end())
    {
        throw generation_error(fmt::format("Impossible to find database: ", m_database_name));
    }

    edc_compilation_unit_writer_t file_writer{*database.begin()};

    std::filesystem::path header_file = output_dir / (base_file_name + c_header_extension);
    std::filesystem::path cpp_file = output_dir / (base_file_name + c_cpp_extension);

    std::ofstream header_out{header_file};
    header_out << file_writer.write_header();
    header_out.close();

    std::string cpp_content = file_writer.write_cpp();
    cpp_content.replace(cpp_content.find("{GENERATED_EDC_HEADER}"), strlen("{GENERATED_EDC_HEADER}"), header_file.filename().string());
    std::ofstream cpp_out{cpp_file};
    cpp_out << cpp_content;
    cpp_out.close();
}

} // namespace generate
} // namespace catalog
} // namespace gaia
