////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <string>

class incubator_helper_t
{
public:
    incubator_helper_t() = delete;
    incubator_helper_t(std::string name);
    void show_status(bool is_on);

private:
    std::string m_instance_name;
};
