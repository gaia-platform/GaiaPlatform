/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

// Export all symbols declared in this file.
#pragma GCC visibility push(default)

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
 * @param gaia_config_file Full path and name of gaia configuration file.
 * @param logger_config_file Full path and name of the logger configuration file.
 * @throw configuration_error
 */
void initialize(const char* gaia_config_file = nullptr, const char* logger_config_file = nullptr);

/**
 *  Shutdown the Gaia sub systems
 */
void shutdown();

/*@}*/
} // namespace system
/*@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
