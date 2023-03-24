#pragma once

/// @file userver/logging/log_extra.hpp
/// @brief @copybrief logging::LogExtra

#include <initializer_list>
#include <string>
#include <utility>
#include <variant>

#include <boost/container/container_fwd.hpp>

#include <userver/compiler/select.hpp>
#include <userver/logging/logger.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {
class Span;
}  // namespace tracing

namespace logging {

class LogHelper;

/// Extra tskv fields storage
class LogExtra final {
 public:
  using Value = std::variant<std::string, int, long, long long, unsigned int,
                             unsigned long, unsigned long long, float, double>;
  using Key = std::string;
  using Pair = std::pair<Key, Value>;

  /// Specifies replacement policy for newly added values
  enum class ExtendType {
    kNormal,  ///< Added value can be replaced
    kFrozen,  ///< Attempts to replace this value will be ignored
  };

  LogExtra() noexcept;

  LogExtra(const LogExtra&);

  // NOLINTNEXTLINE(performance-noexcept-move-constructor)
  LogExtra(LogExtra&&);

  ~LogExtra();

  // NOLINTNEXTLINE(performance-noexcept-move-constructor)
  LogExtra& operator=(LogExtra&&);

  LogExtra& operator=(const LogExtra&);

  /// Constructs LogExtra containing an initial batch of key-value pairs
  LogExtra(std::initializer_list<Pair> initial,
           ExtendType extend_type = ExtendType::kNormal);

  /// Adds a single key-value pair
  void Extend(std::string key, Value value,
              ExtendType extend_type = ExtendType::kNormal);

  /// Adds a single key-value pair
  void Extend(Pair extra, ExtendType extend_type = ExtendType::kNormal);

  /// Adds a batch of key-value pairs
  void Extend(std::initializer_list<Pair> extra,
              ExtendType extend_type = ExtendType::kNormal);

  /// @brief Merges contents of other LogExtra with existing key-value pairs
  /// preserving freeze states
  void Extend(const LogExtra& extra);

  /// @brief Merges contents of other LogExtra with existing key-value pairs
  /// preserving freeze states
  void Extend(LogExtra&& extra);

  /// @brief Creates a LogExtra with current thread's stacktrace if the default
  /// log level is less or equal to DEBUG
  static LogExtra StacktraceNocache() noexcept;

  /// @brief Creates a LogExtra with current thread's stacktrace if the logger
  /// log level is less or equal to DEBUG
  static LogExtra StacktraceNocache(logging::LoggerCRef logger) noexcept;

  /// @brief Creates a LogExtra with current thread's stacktrace if the logger
  /// log level is less or equal to DEBUG. Uses cache for
  /// faster stacktrace symbolization.
  static LogExtra Stacktrace() noexcept;

  /// @brief Creates a LogExtra with current thread's stacktrace if the logger
  /// log level is less or equal to DEBUG. Uses cache for
  /// faster stacktrace symbolization.
  static LogExtra Stacktrace(logging::LoggerCRef logger) noexcept;

  /// @brief Adds a range of key-value pairs
  template <typename Iterator>
  void ExtendRange(Iterator first, Iterator last,
                   ExtendType extend_type = ExtendType::kNormal) {
    // NOTE: must replace existing rewritable values
    for (Iterator it = first; it != last; ++it) Extend(*it, extend_type);
  }

  /// @brief Marks specified value as frozen, all attempts to overwrite it will
  /// be silently ignored.
  void SetFrozen(const std::string& key);

  friend class LogHelper;
  friend class tracing::Span;

 private:
  const Value& GetValue(std::string_view key) const;

  class ProtectedValue final {
   public:
    ProtectedValue() = default;
    ProtectedValue(Value value, bool frozen);
    ProtectedValue(const ProtectedValue& other) = default;
    ProtectedValue(ProtectedValue&& other) = default;

    ProtectedValue& operator=(const ProtectedValue& other);
    ProtectedValue& operator=(ProtectedValue&& other) noexcept;

    void SetFrozen();
    Value& GetValue() { return value_; }
    const Value& GetValue() const { return value_; }

   private:
    Value value_;
    bool frozen_ = false;
  };

  static constexpr std::size_t kSmallVectorSize = 24;
  static constexpr std::size_t kPimplSize = compiler::SelectSize()
                                                .ForLibCpp32(1168)
                                                .ForLibCpp64(1560)
                                                .ForLibStdCpp64(1944)
                                                .ForLibStdCpp32(1356);
  using MapItem = std::pair<Key, ProtectedValue>;
  using Map = boost::container::small_vector<MapItem, kSmallVectorSize>;

  void Extend(std::string key, ProtectedValue protected_value,
              ExtendType extend_type = ExtendType::kNormal);
  void Extend(MapItem extra, ExtendType extend_type = ExtendType::kNormal);

  std::pair<Key, ProtectedValue>* Find(std::string_view);

  const std::pair<Key, ProtectedValue>* Find(std::string_view) const;

  utils::FastPimpl<Map, kPimplSize, alignof(void*)> extra_;
};

}  // namespace logging

USERVER_NAMESPACE_END
