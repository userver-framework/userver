#include "buffered_file_sink.hpp"

#include "open_file_helper.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

BufferedFileSink::BufferedFileSink(const std::string& filename)
    : filename_{filename}, file_(OpenFile<fs::blocking::CFile>(filename)) {
  if (file_.GetSize() > 0) {
    file_.Write("\n");
  }
}

void BufferedFileSink::Reopen(ReopenMode mode) {
  file_.FlushLight();
  auto new_file = OpenFile<fs::blocking::CFile>(filename_, mode);
  std::move(file_).Close();
  file_ = std::move(new_file);
}

BufferedFileSink::~BufferedFileSink() = default;

void BufferedFileSink::Write(std::string_view log) { file_.Write(log); }

void BufferedFileSink::Flush() {
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
