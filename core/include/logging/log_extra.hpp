#pragma once

/// @file logging/log_extra.hpp
/// @brief @copybrief logging::LogExtra

#include <initializer_list>
#include <utility>

#include <boost/container/container_fwd.hpp>
#include <boost/variant.hpp>

#include <utils/fast_pimpl.hpp>

namespace tracing {
class Span;
}

namespace logging {

class LogHelper;

/// Extra tskv fields storage
class LogExtra {
 public:
  using Value = boost::variant<std::string, unsigned, int64_t, uint64_t, int,
                               float, double>;
  using Key = std::string;
  using Pair = std::pair<Key, Value>;

  /// Specifies replacement policy for newly added values
  enum class ExtendType {
    kNormal,  ///< Added value can be replaced
    kFrozen,  ///< Attempts to replace this value will be ignored
  };

  LogExtra() noexcept;

  LogExtra(const LogExtra&);

  LogExtra(LogExtra&&);

  ~LogExtra();

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

  /// @brief Creates a LogExtra with current thread's stacktrace
  static LogExtra StacktraceNocache();

  static LogExtra Stacktrace();

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
  const Value& GetValue(const std::string& key) const;

  class ProtectedValue {
   public:
    ProtectedValue() = default;
    ProtectedValue(Value value, bool frozen);
    ProtectedValue(const ProtectedValue& other) = default;
    ProtectedValue(ProtectedValue&& other) = default;

    ProtectedValue& operator=(const ProtectedValue& other);
    ProtectedValue& operator=(ProtectedValue&& other);

    void SetFrozen();
    Value& GetValue() { return value_; }
    const Value& GetValue() const { return value_; }

   private:
    Value value_;
    bool frozen_ = false;
  };

  static constexpr auto kSmallVectorSize = 24;
#ifdef _LIBCPP_VERSION  // TODO Find out a nicer way to calculate this size
  static constexpr auto kPimplSize = 1560;
#else
  static constexpr auto kPimplSize = 1944;
#endif
  using MapItem = std::pair<Key, ProtectedValue>;
  using Map = boost::container::small_vector<MapItem, kSmallVectorSize>;

  void Extend(std::string key, ProtectedValue protected_value,
              ExtendType extend_type = ExtendType::kNormal);
  void Extend(MapItem extra, ExtendType extend_type = ExtendType::kNormal);

  std::pair<Key, ProtectedValue>* Find(const Key&);

  const std::pair<Key, ProtectedValue>* Find(const Key&) const;

  utils::FastPimpl<Map, kPimplSize, 8, true> extra_;
};

}  // namespace logging
