#include <gtest/gtest.h>

#include <userver/fs/read.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Fs, GetLexicallyRelative) {
  EXPECT_EQ(fs::GetLexicallyRelative("path/to/file", "path/"), "to/file");
  EXPECT_EQ(fs::GetLexicallyRelative("/path/to/file", "/path"), "/to/file");
}

USERVER_NAMESPACE_END
