#pragma once

#if __has_include(<bson/bson.h>)
#include <bson/bson.h>
#elif __has_include(<libbson-1.0/bson.h>)
#include <libbson-1.0/bson.h>
#else
#error "Failed to find BSON header"
#endif
