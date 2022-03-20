#pragma once

#if __has_include(<mongoc/mongoc.h>)
#include <mongoc/mongoc.h>
#elif __has_include(<libmongoc-1.0/mongoc.h>)
#include <libmongoc-1.0/mongoc.h>
#else
#error "Failed to find mongoc.h header"
#endif
