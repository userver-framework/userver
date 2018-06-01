#pragma once

#include <initializer_list>
#include <unordered_map>
#include <utility>

#include <boost/variant.hpp>

namespace logging {

class LogHelper;

class LogExtra {
 public:
  using Value = boost::variant<std::string, unsigned, int64_t, uint64_t, int,
                               float, double>;
  using Key = std::string;
  using Pair = std::pair<Key, Value>;

  enum class ExtendType { kNormal, kFrozen };

  LogExtra() = default;
  LogExtra(std::initializer_list<Pair> initial,
           ExtendType extend_type = ExtendType::kNormal);

  void Extend(std::string key, Value value,
              ExtendType extend_type = ExtendType::kNormal);
  void Extend(Pair extra, ExtendType extend_type = ExtendType::kNormal);
  void Extend(std::initializer_list<Pair> extra,
              ExtendType extend_type = ExtendType::kNormal);
  void Extend(const LogExtra& extra);
  void Extend(LogExtra&& extra);

  template <typename Iterator>
  void ExtendRange(Iterator first, Iterator last,
                   ExtendType extend_type = ExtendType::kNormal) {
    // NOTE: must replace existing rewritable values
    for (Iterator it = first; it != last; ++it) Extend(*it, extend_type);
  }

  template <typename T>
  const T& Get(const std::string& key) const;

  template <typename T>
  const T& Get(const std::string& key, const T& default_value) const;

  void SetFrozen(const std::string& key);

  friend class LogHelper;

 private:
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

  using Map = std::unordered_map<Key, ProtectedValue>;

  void Extend(std::string key, ProtectedValue protected_value,
              ExtendType extend_type = ExtendType::kNormal);
  void Extend(Map::value_type extra,
              ExtendType extend_type = ExtendType::kNormal);

  Map extra_;
};

template <typename T>
const T& LogExtra::Get(const std::string& key) const {
  try {
    const ProtectedValue& protected_value = extra_.at(key);
    return boost::get<T>(protected_value.GetValue());
  } catch (const std::exception& ex) {
    throw std::runtime_error("can't get LogExtra value for key '" + key +
                             "': " + ex.what());
  }
}

template <typename T>
const T& LogExtra::Get(const std::string& key, const T& default_value) const {
  auto it = extra_.find(key);
  if (it == extra_.end()) return default_value;
  try {
    return boost::get<T>(it->second.GetValue());
  } catch (const std::exception& ex) {
    throw std::runtime_error("can't get LogExtra value for key '" + key +
                             "': " + ex.what());
  }
}

}  // namespace logging
