/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <synchronization.hpp>

namespace gaia
{
/**
 * \addtogroup Gaia
 * @{
 */
namespace common
{
/**
 * \addtogroup Common
 * @{
 */

template <class T> class queue_element_t
{

};

template <class T> class queue_t
{
public:

    queue_t();
    ~queue_t<T>();
};

#include "queue.inc"

/*@}*/
}
/*@}*/
}
