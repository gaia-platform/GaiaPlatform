/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/common.hpp"

#include "gaia_internal/common/assert.hpp"
#include "gaia_internal/db/db_test_base.hpp"

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
    explicit db_catalog_test_base_t(
        std::string ddl_file_name = common::c_empty_string,
        bool is_managing_session = true,
        bool start_ddl_session = true,
        bool start_new_session_after_setup = false)
        : db_test_base_t(is_managing_session, start_ddl_session), m_ddl_file_name(std::move(ddl_file_name)), m_start_new_session_after_setup(start_new_session_after_setup)
    {
        ASSERT_PRECONDITION(
            ddl_file_name.empty() || !is_managing_session || start_ddl_session,
            "db_catalog_test_base_t should not use a DDL file without requesting the start of a DDL session!");
        ASSERT_PRECONDITION(
            !start_new_session_after_setup || is_managing_session,
            "db_catalog_test_base_t cannot be asked to start a new session after startup if it was not asked to manage sessions!");
    }

    void SetUp() override
    {
        db_test_base_t::SetUp();

        reset_database_status();

        if (!m_ddl_file_name.empty())
        {
            if (!is_managing_session())
            {
                begin_ddl_session();
            }

            load_schema(m_ddl_file_name);

            if (!is_managing_session())
            {
                end_session();
            }
        }

        if (m_start_new_session_after_setup)
        {
            end_session();
            begin_session();
        }
    }

    static void reset_database_status();
    void load_schema(std::string ddl_file_name);

private:
    std::string m_ddl_file_name;
    bool m_start_new_session_after_setup;
};

} // namespace db
} // namespace gaia
