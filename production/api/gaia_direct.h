/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

/**
 * All objects that are managed by the gaia platform must inherit from gaia_base
 */
namespace gaia_api {
    /**
     * user object representing the payload
     */ 
    class gaia_base {
        virtual ~gaia_base() = default;
    };

    /**
     * record locator
     */
    struct gaia_record {
    };

    /**
     * field locator
     */
    struct gaia_field {
    };
};