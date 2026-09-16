#include "file_sink.hpp"

#include "open_file_helper.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

FileSink::FileSink(const std::string& filename)
    : FdSink(OpenFile<fs::blocking::FileDescriptor>(filename)),
      filename_{filename}
{
    if (GetFd().GetSize() > 0) {
        GetFd().Write("\n");
    }
}

void FileSink::Reopen(ReopenMode mode) {
    // We do not do the `GetFd().FSync()` as it makes no sense

    auto new_file = OpenFile<fs::blocking::FileDescriptor>(filename_, mode);
    std::move(GetFd()).Close();
    SetFd(std::move(new_file));
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
