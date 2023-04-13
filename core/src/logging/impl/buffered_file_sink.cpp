#include "buffered_file_sink.hpp"

#include <cstdio>

#include <spdlog/pattern_formatter.h>
#include <boost/filesystem.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

BufferedFileSink::BufferedFileSink(const std::string& filename)
    : filename_{filename}, file_(OpenFile<fs::blocking::CFile>(filename)) {
  if (file_.GetSize() > 0) {
    file_.Write("\n");
  }
}

void BufferedFileSink::Reopen(ReopenMode mode) {
  std::lock_guard lock{GetMutex()};
  file_.Flush();
  std::move(file_).Close();
  file_ = OpenFile<fs::blocking::CFile>(filename_, mode);
}

BufferedFileSink::~BufferedFileSink() = default;

void BufferedFileSink::Write(std::string_view log) { file_.Write(log); }

void BufferedFileSink::flush() {
  std::lock_guard lock(GetMutex());
  if (file_.IsOpen()) {
    file_.Flush();
  }
}

BufferedFileSink::BufferedFileSink(fs::blocking::CFile&& file)
    : file_(std::move(file)) {}

fs::blocking::CFile& BufferedFileSink::GetFile() { return file_; }

BufferedStdoutFileSink::BufferedStdoutFileSink()
    : BufferedFileSink{fs::blocking::CFile(stdout)} {}

BufferedStderrFileSink::BufferedStderrFileSink()
    : BufferedFileSink{fs::blocking::CFile(stderr)} {}

void BufferedStdoutFileSink::flush() {}

void BufferedStderrFileSink::flush() {}

void BufferedStdoutFileSink::Reopen(ReopenMode) {}

void BufferedStderrFileSink::Reopen(ReopenMode) {}

BufferedStdoutFileSink::~BufferedStdoutFileSink() {
  if (GetFile().IsOpen()) {
    std::move(GetFile()).Release();
  }
}

BufferedStderrFileSink::~BufferedStderrFileSink() {
  if (GetFile().IsOpen()) {
    std::move(GetFile()).Release();
  }
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
