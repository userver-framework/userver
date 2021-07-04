#pragma once

#include <vector>

#include <userver/storages/postgres/io/nullable_traits.hpp>
#include <userver/storages/postgres/io/traits.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>

#include <userver/storages/postgres/io/supported_types.hpp>

#include <userver/storages/postgres/null.hpp>

namespace storages {
namespace postgres {
namespace detail {

/// @brief Helper to write query parameters to buffers
class QueryParameters {
 public:
  bool Empty() const { return param_types.empty(); }
  std::size_t Size() const { return param_types.size(); }
  const char* const* ParamBuffers() const { return param_buffers.data(); }
  const Oid* ParamTypesBuffer() const { return param_types.data(); }
  const int* ParamLengthsBuffer() const { return param_lengths.data(); }
  const int* ParamFormatsBuffer() const { return param_formats.data(); }

  template <typename T>
  void Write(const UserTypes& types, const T& arg) {
    static_assert(io::traits::kIsMappedToPg<T>,
                  "Type doesn't have mapping to Postgres type");
    WriteParamType(types, arg);
    WriteNullable(types, arg, io::traits::IsNullable<T>{});
  }

  template <typename... T>
  void Write(const UserTypes& types, const T&... args) {
    (Write(types, args), ...);
  }

  std::size_t TypeHash() const;

 private:
  template <typename T>
  void WriteParamType(const UserTypes& types, const T&) {
    // C++ to pg oid mapping
    param_types.push_back(io::CppToPg<T>::GetOid(types));
  }

  template <typename T>
  void WriteNullable(const UserTypes& types, const T& arg, std::true_type) {
    using NullDetector = io::traits::GetSetNull<T>;
    if (NullDetector::IsNull(arg)) {
      param_formats.push_back(io::kPgBinaryDataFormat);
      param_lengths.push_back(io::kPgNullBufferSize);
      param_buffers.push_back(nullptr);
    } else {
      WriteNullable(types, arg, std::false_type{});
    }
  }

  template <typename T>
  void WriteNullable(const UserTypes& types, const T& arg, std::false_type) {
    param_formats.push_back(io::kPgBinaryDataFormat);
    parameters.push_back({});
    auto& buffer = parameters.back();
    io::WriteBuffer(types, buffer, arg);
    auto size = buffer.size();
    param_lengths.push_back(size);
    if (size == 0) {
      param_buffers.push_back(empty_buffer);
    } else {
      param_buffers.push_back(buffer.data());
    }
  }

 private:
  using OidList = std::vector<Oid>;
  using BufferType = std::vector<char>;
  using ParameterList = std::vector<BufferType>;
  using IntList = std::vector<int>;

  static constexpr const char* empty_buffer = "";

  ParameterList parameters;  // TODO Replace with a single buffer
  OidList param_types;
  std::vector<const char*> param_buffers;
  IntList param_lengths;
  IntList param_formats;
};

}  // namespace detail
}  // namespace postgres
}  // namespace storages
