/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

/**
 * A repository of schemas. This class contains a mapping between the schema name and the
 * path on the disk.
 *
 *   addr_book.ddl => /home/simone/repos/GaiaPlatform/production/cmake-build-debug/schemas/addr_book.ddl
 *
 * If no mapping exists for a schema name this class will try to find the schema in a list of search paths.
 * By default this class "registers" the path of the gaia schemas used for testing.
 */
class schema_repository_t
{
public:
    schema_repository_t(const schema_repository_t&) = delete;
    schema_repository_t& operator=(const schema_repository_t&) = delete;
    schema_repository_t(schema_repository_t&&) = delete;
    schema_repository_t& operator=(schema_repository_t&&) = delete;

    static schema_repository_t& instance()
    {
        static schema_repository_t schema_manager;
        return schema_manager;
    }

    /**
     * Given a schema name search for it in the search paths.
     *
     * @param schema_name Name of the schema to search (eg. addr_book.ddl).
     * @return If found, an optional with the absolute path, an empty optional otherwise.
     */
    std::optional<std::string> find_schema(std::string schema_name);

    /**
     * Loads the schema identified by schema_name into the catalog.
     */
    void load_schema(std::string schema_name);

    /**
     * Path that will be used by the method search_schema to find schemas.
     * @param path Absolute path.
     */
    void add_search_path(std::string path);

    /**
     * Clear the mapping between the schema names and thir paths.
     */
    void clear_schema_cache();

private:
    schema_repository_t();

    std::vector<std::filesystem::path> m_search_paths;
    std::map<std::string, std::filesystem::path> m_schemas_cache;

    /**
     * Automatically adds the gaia paths for the well-known schemas.
     */
    void add_gaia_search_paths();
};
