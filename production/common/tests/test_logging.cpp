/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include "gtest/gtest.h"
#include "gaia_logging.h"

namespace gaia::common::logging {

TEST(common, logging) {
    init_logging("");

    gaia_logger_t logger(c_gaia_root_logger);

    logger.critical("critical");
}

} // namespace gaia::common::logging