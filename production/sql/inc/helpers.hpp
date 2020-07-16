#pragma once

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) ((size_t)(sizeof(x) / sizeof((x)[0])))
#endif
