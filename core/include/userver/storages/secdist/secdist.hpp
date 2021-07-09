#pragma once

/// @file storages/secdist/secdist.hpp
/// @brief @copybrief storages::secdist::SecdistConfig

#include <any>
#include <chrono>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <userver/formats/json/value.hpp>

/// Credentials storage
namespace storages::secdist {

class SecdistConfig;

namespace detail {

template <typename T>
class SecdistModule final {
 public:
  static const T& Get(const SecdistConfig& config);
  static std::any Factory(const formats::json::Value& data) { return T(data); }

 private:
  static std::size_t index_;
};

}  // namespace detail

// clang-format off

/// @ingroup userver_clients
///
/// @brief Client to retrieve credentials from the components::Secdist.
///
/// ## Example usage:
///
/// Declare a type that would work with the credentials:
///
/// @snippet storages/secdist/secdist_test.cpp Secdist Usage Sample - UserPasswords
///
/// Fill the components::Secdist `config` file from with the secure data:
///
/// @snippet storages/secdist/secdist_test.cpp Secdist Usage Sample - UserPasswords
///
/// Retrieve SecdistConfig from components::Secdist and get the type from it:
///
/// @snippet storages/secdist/secdist_test.cpp Secdist Usage Sample - SecdistConfig

// clang-format on
class SecdistConfig final {
 public:
  SecdistConfig();
  explicit SecdistConfig(const std::string& path, bool missing_ok);

  template <typename T>
  static std::size_t Register(
      std::function<std::any(const formats::json::Value&)>&& factory) {
    return Register(std::move(factory));
  }

  template <typename T>
  const T& Get() const {
    return detail::SecdistModule<T>::Get(*this);
  }

 private:
  void Init(const formats::json::Value& doc);

  static std::size_t Register(
      std::function<std::any(const formats::json::Value&)>&& factory);
  const std::any& Get(const std::type_index& type, std::size_t index) const;

  template <typename T>
  friend class detail::SecdistModule;

 private:
  std::vector<std::any> configs_;
};

namespace detail {

template <typename T>
const T& SecdistModule<T>::Get(const SecdistConfig& config) {
  return std::any_cast<const T&>(config.Get(typeid(T), index_));
}

template <typename T>
std::size_t SecdistModule<T>::index_ =
    SecdistConfig::Register<T>(&SecdistModule<T>::Factory);

}  // namespace detail

}  // namespace storages::secdist
