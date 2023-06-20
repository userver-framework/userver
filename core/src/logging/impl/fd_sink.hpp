#pragma once

#include <string_view>

#include <userver/fs/blocking/file_descriptor.hpp>

#include "base_sink.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

class FdSink : public BaseSink {
 public:
  FdSink() = delete;
  explicit FdSink(fs::blocking::FileDescriptor fd);

  ~FdSink() override;

  void Flush() override;

 protected:
  void Write(std::string_view log) final;
  fs::blocking::FileDescriptor& GetFd();
  void SetFd(fs::blocking::FileDescriptor&& fd);

 private:
  fs::blocking::FileDescriptor fd_;
};

class StdoutSink final : public FdSink {
 public:
  StdoutSink();
  ~StdoutSink() final;
  void Flush() final;
};

class StderrSink final : public FdSink {
 public:
  StderrSink();
  ~StderrSink() final;
  void Flush() final;
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
