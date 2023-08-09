#include "buffered_file_sink.hpp"

#include <userver/fs/blocking/c_file.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/utest/utest.hpp>

#include "sink_helper_test.hpp"

USERVER_NAMESPACE_BEGIN

UTEST(BufferedFileSink, UnownedSinkLog) {
  const auto file_scope = fs::blocking::TempFile::Create();

  {
    fs::blocking::CFile file_stream{file_scope.GetPath(),
                                    fs::blocking::OpenFlag::kWrite};

    auto sink = logging::impl::BufferedUnownedFileSink(file_stream.GetNative());
    EXPECT_NO_THROW(sink.Log({"message\n", logging::Level::kWarning}));
  }

  EXPECT_EQ(test::ReadFromFile(file_scope.GetPath()),
            test::Messages("message"));
}

USERVER_NAMESPACE_END
