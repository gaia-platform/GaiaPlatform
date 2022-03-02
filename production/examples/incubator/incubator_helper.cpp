////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "incubator_helper.hpp"

#include <iostream>

incubator_helper_t::incubator_helper_t(std::string name)
    : m_instance_name(name)
{
}

void incubator_helper_t::show_status(bool is_on)
{
    std::cout << "[" << m_instance_name << " instance callback]: is_on == " << is_on << std::endl;
}