#pragma once

#include <cstdio>
#include <mutex>
#include <string_view>

#include <spdlog/sinks/sink.h>

#include <logging/impl/base_sink.hpp>
#include <userver/fs/blocking/c_file.hpp>

#include "open_file_helper.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

class BufferedFileSink : public BaseSink {
 public:
  explicit BufferedFileSink(const std::string& filename);
  ~BufferedFileSink() override;

  void Reopen(ReopenMode mode) override;

  void Flush() override;

 protected:
  explicit BufferedFileSink(fs::blocking::CFile&& file);

  void Write(std::string_view log) final;

  fs::blocking::CFile& GetFile();

 private:
  std::string filename_;
  fs::blocking::CFile file_;
};

class BufferedUnownedFileSink final : public BufferedFileSink {
 public:
  explicit BufferedUnownedFileSink(std::FILE* c_file);
  ~BufferedUnownedFileSink() override;
  void Reopen(ReopenMode) override;
  void Flush() override;
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
