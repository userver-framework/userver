#pragma once

#include <type_traits>

#include <grpcpp/support/config.h>
#include <grpcpp/support/string_ref.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

inline std::string ToString(grpc::string_ref str) { return {str.data(), str.size()}; }

inline decltype(auto) ToString(const grpc::string& str) {
    if constexpr (std::is_same_v<grpc::string, std::string>) {
        return str;
    } else {
        return std::string{str};
    }
}

inline std::string ToString(grpc::string&& str) {
    if constexpr (std::is_same_v<grpc::string, std::string>) {
        return std::move(str);
    } else {
        return std::string{str.data(), str.size()};
    }
}

inline std::string_view ToStringView(grpc::string_ref str) { return {str.data(), str.size()}; }

inline decltype(auto) ToGrpcString(const std::string& str) {
    if constexpr (std::is_same_v<grpc::string, std::string>) {
        return str;
    } else {
        return grpc::string{str};
    }
}

inline grpc::string ToGrpcString(std::string&& str) {
    if constexpr (std::is_same_v<grpc::string, std::string>) {
        return std::move(str);
    } else {
        return grpc::string{std::move(str)};
    }
}

inline grpc::string ToGrpcString(std::string_view str) { return grpc::string{str}; }

inline grpc::string_ref ToGrpcStringRef(std::string_view str) { return grpc::string_ref{str.data(), str.size()}; }

inline grpc::string_ref ToGrpcStringRef(const std::string& str) { return grpc::string_ref{str.data(), str.size()}; }

inline grpc::string ToGrpcString(grpc::string_ref str) { return {str.data(), str.size()}; }

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
