/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

typedef enum : uint8_t
{
    undefined = 0,
    // Cannot use stranger otherwise it will conflict with catalog table strangers.
    sconosciuto = 1,
    student = 2,
    parent = 3,
    staff = 4
} person_type;

