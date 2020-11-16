/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "db_test_base.hpp"

namespace gaia
{
namespace db
{

/**
 * Base class for tests that require a schema to run.
 */
class db_catalog_test_base_t : public db_test_base_t
{
protected:
    /**
     * @param ddl_file_name The ddl filename, not the path. Eg. "addr_book.ddl"
     */
    explicit db_catalog_test_base_t(std::string ddl_file_name, bool client_manages_session = false)
        : db_test_base_t(client_manages_session)
        , m_dd_file_name(std::move(ddl_file_name)){};

    void SetUp() override
    {
        db_test_base_t::SetUp();

        if (is_client_manage_sessions())
        {
            begin_session();
        }

        reset_database_status();
        load_dll(m_dd_file_name);

        if (is_client_manage_sessions())
        {
            end_session();
        }
    }

private:
    std::string m_dd_file_name;

    void reset_database_status();
    void load_dll(std::string ddl_file_name);
};

} // namespace db
} // namespace gaia
