////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

enum status_code
{
    idle = 0,
    success = 1,
    failure = 2,
    running = 3
};
enum node_type
{
    selector = 1,
    sequence = 2,
    action = 3
};
enum action_code
{
    check_temperature = 1,
    set_fan_speed = 2,
    check_fan_speed = 3,
    adjust_fan_speed = 4
};
