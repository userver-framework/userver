#pragma once

#include <string_view>

#include <userver/fs/blocking/file_descriptor.hpp>

#include "base_sink.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

class FdSink : public BaseSink {
 public:
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

class UnownedFdSink final : public FdSink {
 public:
  explicit UnownedFdSink(int fd);
  ~UnownedFdSink() override;

  void Flush() override;
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
