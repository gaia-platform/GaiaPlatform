/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "schema_repository.hpp"

#include <filesystem>

#include "ddl_execution.hpp"
#include "logger.hpp"

namespace fs = std::filesystem;

const std::string c_schemas_folder_name = "schemas";
const std::string c_test_folder_name = "test";

schema_repository_t::schema_repository_t()
{
    add_gaia_search_paths();
}

std::optional<std::string> schema_repository_t::find_schema(std::string schema_name)
{
    auto it = m_schemas_cache.find(schema_name);

    if (it != m_schemas_cache.end())
    {
        fs::path schema_path = it->second;

        if (fs::exists(schema_path))
        {
            return schema_path.string();
        }

        m_schemas_cache.erase(schema_name);
    }

    for (const fs::path& search_path : m_search_paths)
    {
        auto schema_path = fs::path(search_path) / schema_name;

        gaia_log::sys().debug("Schema path: {}", schema_path.string());

        if (fs::exists(schema_path))
        {
            m_schemas_cache.insert({schema_name, schema_path});
            return schema_path.string();
        }
    }

    return std::nullopt;
}

void schema_repository_t::load_schema(std::string schema_name)
{
    gaia_log::sys().debug("Loading schema {}", schema_name);

    std::optional<std::string> schema_path = find_schema(schema_name);

    if (!schema_path)
    {
        throw new std::invalid_argument("Impossible to find schema for name: " + schema_name);
    }

    gaia::catalog::load_catalog(schema_path.value().c_str());
}

void schema_repository_t::add_search_path(std::string directory)
{
    gaia_log::sys().debug("Adding search path {}", directory);

    fs::path path = fs::path(directory);

    if (!fs::is_directory(path))
    {
        throw std::invalid_argument(directory + " is not a folder");
    }

    m_search_paths.push_back(fs::absolute(path));
}

void schema_repository_t::clear_schema_cache()
{
    m_schemas_cache.clear();
}

void schema_repository_t::add_gaia_search_paths()
{
    fs::path current_path = fs::current_path();
    fs::path schemas_path = fs::path(current_path) / c_schemas_folder_name;

    while (!fs::exists(schemas_path) && current_path.has_root_path() && current_path != current_path.root_path())
    {
        current_path = current_path.parent_path();
        schemas_path = fs::path(current_path) / c_schemas_folder_name;
    }

    if (!fs::exists(schemas_path))
    {
        return;
    }

    add_search_path(schemas_path.string());

    auto tests_path = schemas_path / c_test_folder_name;

    if (!fs::exists(tests_path))
    {
        return;
    }

    add_search_path(tests_path.string());
}
