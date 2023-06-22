#include "buffered_file_sink.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

BufferedFileSink::BufferedFileSink(const std::string& filename)
    : filename_{filename}, file_(OpenFile<fs::blocking::CFile>(filename)) {
  if (file_.GetSize() > 0) {
    file_.Write("\n");
  }
}

void BufferedFileSink::Reopen(ReopenMode mode) {
  const std::lock_guard lock{GetMutex()};
  file_.FlushLight();
  std::move(file_).Close();
  file_ = OpenFile<fs::blocking::CFile>(filename_, mode);
}

BufferedFileSink::~BufferedFileSink() = default;

void BufferedFileSink::Write(std::string_view log) { file_.Write(log); }

void BufferedFileSink::Flush() {
  const std::lock_guard lock(GetMutex());
  if (file_.IsOpen()) {
    file_.FlushLight();
  }
}

BufferedFileSink::BufferedFileSink(fs::blocking::CFile&& file)
    : file_(std::move(file)) {}

fs::blocking::CFile& BufferedFileSink::GetFile() { return file_; }

BufferedUnownedFileSink::BufferedUnownedFileSink(std::FILE* c_file)
    : BufferedFileSink{fs::blocking::CFile(c_file)} {}

void BufferedUnownedFileSink::Flush() {}

void BufferedUnownedFileSink::Reopen(ReopenMode) {}

BufferedUnownedFileSink::~BufferedUnownedFileSink() {
  if (GetFile().IsOpen()) {
    std::move(GetFile()).Release();
  }
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
