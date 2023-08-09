#pragma once

#include <string>
#include <vector>

#include <userver/engine/io/socket.hpp>
#include <userver/fs/blocking/file_descriptor.hpp>

USERVER_NAMESPACE_BEGIN

namespace test {

std::vector<std::string> NormalizeLogs(const std::string& data);

std::vector<std::string> ReadFromFile(const std::string& filename);

std::vector<std::string> ReadFromFd(fs::blocking::FileDescriptor&& fd);

std::vector<std::string> ReadFromSocket(engine::io::Socket&& sock);

template <typename... Strings>
std::vector<std::string> Messages(const Strings&... strings) {
  return {strings...};
}

}  // namespace test

USERVER_NAMESPACE_END
