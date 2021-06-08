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
 * Utility class that given a schema file name (eg. "addr_book.ddl") search for it into a set of 
 * predefined folders, and loads its content into the Catalog.
 */
class schema_loader_t
{
public:
    schema_loader_t(const schema_loader_t&) = delete;
    schema_loader_t& operator=(const schema_loader_t&) = delete;
    schema_loader_t(schema_loader_t&&) = delete;
    schema_loader_t& operator=(schema_loader_t&&) = delete;

    static schema_loader_t& instance()
    {
        static schema_loader_t schema_manager;
        return schema_manager;
    }

    /**
     * Loads the schema identified by schema_file_name into the catalog.
     *
     * @see find_schema()
     * @throws std::invalid_argument if the schema is not found.
     */
    void load_schema(std::string schema_file_name);

    /**
     * Given a schema name search for it both locally and in the search paths.
     * The first match is returned.
     *
     * 1. schema_file_name is considered as a normal file. It can be bo both
     *    a relative or an absolute path.
     * 2. If not found, search schema_file_name in the "search paths".
     * 3. If not found, throws an exception.
     *
     * @param schema_file_name Name of the schema to search (eg. addr_book.ddl).
     * @return If found, an optional with the absolute path, an empty optional otherwise.
     */
    std::optional<std::string> find_schema(std::string schema_file_name);

    /**
     * Path that will be used by the method search_schema to find schemas.
     * @param path Absolute path.
     */
    void add_search_path(std::string path);

    /**
     * Clear the mapping between the schema names and their paths.
     */
    void clear_schema_cache();

private:
    schema_loader_t();

    std::vector<std::filesystem::path> m_search_paths;
    std::map<std::string, std::filesystem::path> m_schemas_cache;

    /**
     * Automatically adds the paths for the schemas stored in the "schemas/test" directory.
     * The search starts from the current working directory (usually the CMake build directory)
     * and proceeds backwards into the search path until it finds the "schemas" directory.
     * If the "schemas" directory is not found nothing is added.
     */
    void add_gaia_search_paths();
};
