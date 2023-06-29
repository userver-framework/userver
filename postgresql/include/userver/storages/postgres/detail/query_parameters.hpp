#pragma once

#include <vector>

#include <userver/storages/postgres/io/nullable_traits.hpp>
#include <userver/storages/postgres/io/traits.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>

#include <userver/storages/postgres/io/supported_types.hpp>

#include <userver/storages/postgres/null.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

/// @brief Helper to write query parameters to buffers
class QueryParameters {
 public:
  QueryParameters() = default;

  template <class ParamsHolder>
  explicit QueryParameters(ParamsHolder& ph)
      : size_(ph.Size()),
        types_(ph.ParamTypesBuffer()),
        values_(ph.ParamBuffers()),
        lengths_(ph.ParamLengthsBuffer()),
        formats_(ph.ParamFormatsBuffer()) {}

  bool Empty() const { return size_ == 0; }
  std::size_t Size() const { return size_; }
  const char* const* ParamBuffers() const { return values_; }
  const Oid* ParamTypesBuffer() const { return types_; }
  const int* ParamLengthsBuffer() const { return lengths_; }
  const int* ParamFormatsBuffer() const { return formats_; }

  std::size_t TypeHash() const;

 private:
  std::size_t size_ = 0;
  const Oid* types_ = nullptr;
  const char* const* values_ = nullptr;
  const int* lengths_ = nullptr;
  const int* formats_ = nullptr;
};

template <std::size_t ParamsCount>
class StaticQueryParameters {
 public:
  StaticQueryParameters() = default;
  StaticQueryParameters(const StaticQueryParameters&) = delete;
  StaticQueryParameters(StaticQueryParameters&&) = delete;
  StaticQueryParameters& operator=(const StaticQueryParameters&) = delete;
  StaticQueryParameters& operator=(StaticQueryParameters&&) = delete;

  std::size_t Size() const { return ParamsCount; }
  const char* const* ParamBuffers() const { return param_buffers; }
  const Oid* ParamTypesBuffer() const { return param_types; }
  const int* ParamLengthsBuffer() const { return param_lengths; }
  const int* ParamFormatsBuffer() const { return param_formats; }

  template <typename T>
  void Write(std::size_t index, const UserTypes& types, const T& arg) {
    static_assert(io::traits::kIsMappedToPg<T> || std::is_enum_v<T>,
                  "Type doesn't have mapping to Postgres type.");
    static_assert(
        io::traits::kIsMappedToPg<T> || !std::is_enum_v<T>,
        "Type doesn't have mapping to Postgres type. "
        "Enums should be either streamed as their underlying value via the "
        "`template<> struct CanUseEnumAsStrongTypedef<T>: std::true_type {};` "
        "specialization or as a PostgreSQL datatype via the "
        "`template<> struct CppToUserPg<T> : EnumMappingBase<R> { ... };` "
        "specialization. "
        "See page `uPg: Supported data types` for more information.");
    WriteParamType(index, types, arg);
    WriteNullable(index, types, arg, io::traits::IsNullable<T>{});
  }

  template <typename... T>
  void Write(const UserTypes& types, const T&... args) {
    std::size_t index = 0;
    (Write(index++, types, args), ...);
  }

 private:
  template <typename T>
  void WriteParamType(std::size_t index, const UserTypes& types, const T&) {
    // C++ to pg oid mapping
    param_types[index] = io::CppToPg<T>::GetOid(types);
  }

  template <typename T>
  void WriteNullable(std::size_t index, const UserTypes& types, const T& arg,
                     std::true_type) {
    using NullDetector = io::traits::GetSetNull<T>;
    if (NullDetector::IsNull(arg)) {
      param_formats[index] = io::kPgBinaryDataFormat;
      param_lengths[index] = io::kPgNullBufferSize;
      param_buffers[index] = nullptr;
    } else {
      WriteNullable(index, types, arg, std::false_type{});
    }
  }

  template <typename T>
  void WriteNullable(std::size_t index, const UserTypes& types, const T& arg,
                     std::false_type) {
    param_formats[index] = io::kPgBinaryDataFormat;
    auto& buffer = parameters[index];
    io::WriteBuffer(types, buffer, arg);
    auto size = buffer.size();
    param_lengths[index] = size;
    if (size == 0) {
      param_buffers[index] = empty_buffer;
    } else {
      param_buffers[index] = buffer.data();
    }
  }

  using OidList = Oid[ParamsCount];
  using BufferType = std::string;
  using ParameterList = BufferType[ParamsCount];
  using IntList = int[ParamsCount];

  static constexpr const char* empty_buffer = "";

  ParameterList parameters{};
  OidList param_types{};
  const char* param_buffers[ParamsCount]{};
  IntList param_lengths{};
  IntList param_formats{};
};

template <>
class StaticQueryParameters<0> {
 public:
  static std::size_t Size() { return 0; }
  static const char* const* ParamBuffers() { return nullptr; }
  static const Oid* ParamTypesBuffer() { return nullptr; }
  static const int* ParamLengthsBuffer() { return nullptr; }
  static const int* ParamFormatsBuffer() { return nullptr; }

  static void Write(const UserTypes& /*types*/) {}
};

class DynamicQueryParameters {
 public:
  DynamicQueryParameters() = default;
  DynamicQueryParameters(const DynamicQueryParameters&) = delete;
  DynamicQueryParameters(DynamicQueryParameters&&) = default;
  DynamicQueryParameters& operator=(const DynamicQueryParameters&) = delete;
  DynamicQueryParameters& operator=(DynamicQueryParameters&&) = default;

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

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
