/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia_internal/db/db.hpp"
#include "gaia_internal/db/db_test_base.hpp"

namespace gaia
{
namespace db
{

// Base class for tests that execute DDL directly
// rather than having a schema compiled for them.
class db_ddl_test_base_t : public db_test_base_t
{
protected:
    void SetUp() override
    {
        db_test_base_t::SetUp();

        // Close the regular session opened by db_test_base_t::SetUp()
        // and start a DDL session.
        end_session();
        begin_ddl_session();
    }
};

} // namespace db
} // namespace gaia
