#pragma once

#include "fd_sink.hpp"
#include "open_file_helper.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

class FileSink final : public FdSink {
 public:
  explicit FileSink(const std::string& filename);

  void Reopen(ReopenMode mode) final;

 private:
  std::string filename_;
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
