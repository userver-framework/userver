#pragma once

#include "fd_sink.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

class FileSink final : public FdSink {
 public:
  explicit FileSink(const std::string& filename);

  enum class ReopenMode {
    kAppend,
    kTruncate,
  };

  void Reopen(ReopenMode mode);

 private:
  std::string filename_;
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
