/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

namespace gaia
{
/**
 * \addtogroup Gaia
 * @{
 */
namespace system
{
/**
 * \addtogroup System
 * @{
 *
 * Provides system wide facilities for the Gaia Platform
 */

/**
 *  Initialize the Gaia sub systems
 * @param gaia_config_file Full path and name of gaia configuration file
 * @throw configuration_error
 */
void initialize(const char* gaia_config_file = nullptr);

/*@}*/
} // namespace system
/*@}*/
} // namespace gaia
