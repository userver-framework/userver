#pragma once

//  NOLINTBEGIN

static char* volatile mark;

#define TEST_INIT(value) \
    { mark = reinterpret_cast<char*>(&value); }

#define TEST_EXPR(value, expected) \
    { ++mark; }

#define TEST_DEINIT(value) \
    { mark = reinterpret_cast<char*>(&value); }

//  NOLINTEND
