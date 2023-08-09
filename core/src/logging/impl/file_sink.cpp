#include "file_sink.hpp"

#include "open_file_helper.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

FileSink::FileSink(const std::string& filename)
    : FdSink(OpenFile<fs::blocking::FileDescriptor>(filename)),
      filename_{filename} {
  if (GetFd().GetSize() > 0) {
    GetFd().Write("\n");
  }
}

void FileSink::Reopen(ReopenMode mode) {
  GetFd().FSync();
  std::move(GetFd()).Close();
  SetFd(OpenFile<fs::blocking::FileDescriptor>(filename_, mode));
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
