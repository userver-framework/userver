#pragma once

#include <string>

#include <arpa/inet.h>

#include <utils/check_syscall.hpp>
#include <userver/utils/ip/address_base.hpp>
#include <userver/utils/ip/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::ip::detail {

/// @brief Convert IPv4/IPv6 address from std::string
/// @throw AddressSystemError
/// @note use std::string, because inet_ntop require null terminated string 
template <size_t N, typename = std::enable_if_t<kIsValidAddressSize<N>>>
AddressBase<N> AddressFromString(const std::string& str) {
  typename AddressBase<N>::BytesType bytes;
  const auto compare = [](const int& ret, const int& err_mark) {
    return ret <= err_mark;
  };
  const auto family = N == 4 ? AF_INET : AF_INET6;
  utils::CompareSyscallWithCustomException<AddressSystemError>(
      compare, ::inet_pton(family, str.data(), &bytes), 0,
      fmt::format("converting {} address from string",
                  N == 4 ? "IPv4" : "IPv6"));
  return AddressBase<N>(bytes);
}

/// @brief Convert IPv4/IPv6 address to std::string
/// @throw AddressSystemError 
template <size_t N, typename = std::enable_if_t<kIsValidAddressSize<N>>>
std::string AddressToString(const AddressBase<N>& address) {
  constexpr auto addr_len = N == 4 ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN;
  char addr_str[addr_len];
  const auto bytes = address.GetBytes();
  const auto family = N == 4 ? AF_INET : AF_INET6;
  return utils::CheckSyscallNotEqualsCustomException<AddressSystemError>(
      ::inet_ntop(family, &bytes, addr_str, addr_len), nullptr,
      "converting {} address to string", N == 4 ? "IPv4" : "IPv6");
}

}  // namespace utils::ip::detail

USERVER_NAMESPACE_END
