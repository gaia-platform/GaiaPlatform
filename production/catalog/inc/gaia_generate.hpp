//
// Created by simone on 5/31/21.
//

#pragma once

#include <filesystem>
#include <string>

#include <gaia/exception.hpp>

#include <gaia_internal/common/retail_assert.hpp>

namespace gaia
{
namespace catalog
{
namespace generate
{

class generation_error : common::gaia_exception
{
public:
    explicit generation_error(const std::string& message);
};

class gaia_edc_generator_t
{
public:
    gaia_edc_generator_t(std::string database_name)
        : m_database_name(database_name)
    {
        ASSERT_PRECONDITION(!database_name.empty(), "A database name must be specified!");
    }

    void generate(std::filesystem::path header_file, std::filesystem::path cpp_file);

private:
    std::string m_database_name;
};

} // namespace generate
} // namespace catalog
} // namespace gaia
