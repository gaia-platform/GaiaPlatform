////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <string>

#include "gaia/exception.hpp"

namespace gaia
{
/**
 * \addtogroup gaia
 * @{
 */
namespace common
{
/**
 * \addtogroup common
 * @{
 */

/**
 * The exception class used for file open errors.
 */
class file_open_error : public gaia::common::gaia_exception
{
public:
    explicit file_open_error(const char* filename);
};

/**
 * A helper class for loading data from a file.
 */
class file_loader_t
{
public:
    file_loader_t();
    ~file_loader_t();

    size_t load_file_data(const std::string& filename, bool enable_text_mode = false);

    uint8_t* get_data();
    size_t get_data_length();

protected:
    void initialize();
    void clear();

    std::string m_filename;
    uint8_t* m_data;
    size_t m_data_length;
};

/*@}*/
} // namespace common
/*@}*/
} // namespace gaia
