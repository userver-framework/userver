#pragma once

/// @file userver/server/middlewares/configuration.hpp
/// @brief Utility functions/classes for middleware pipelines configuration

#include <string>
#include <vector>

#include <userver/components/loggable_component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {
class ComponentList;
}

namespace server::middlewares {

/// @ingroup userver_middlewares
///
/// @brief Returns a list of middleware-components which userver uses by
/// default in http server.
///
/// The list contains a bunch of middlewares into which most of
/// http-handler functionality is split (metrics, tracing, deadline-propagation
/// etc. etc.)
components::ComponentList DefaultMiddlewareComponents();

/// @ingroup userver_middlewares
///
/// @brief Returns a list of middleware-components required by userver to start
/// a http server.
///
/// Components in this list don't have any useful functionality, they are
/// just infrastructure.
components::ComponentList MinimalMiddlewareComponents();

using MiddlewaresList = std::vector<std::string>;

/// @ingroup userver_middlewares
///
/// @brief Returns the default userver-provided middleware pipeline.
MiddlewaresList DefaultPipeline();

/// @ingroup userver_middlewares userver_base_classes
///
/// @brief Base class to build a server-wide middleware pipeline.
/// One may inherit from it and implement any custom logic, if desired.
class PipelineBuilder : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName{
      "default-server-middleware-pipeline-builder"};

  PipelineBuilder(const components::ComponentConfig&,
                  const components::ComponentContext&);

  /// @brief The method to build a server-wide middleware pipeline,
  /// one may override it if custom behavior is desired.
  /// @param userver_middleware_pipeline default userver-provided middleware
  /// pipeline
  ///
  /// @note We recommend against omitting/modifying default userver pipeline,
  /// but leave a possibility to do so.
  virtual MiddlewaresList BuildPipeline(
      MiddlewaresList userver_middleware_pipeline) const {
    auto& resulting_pipeline = userver_middleware_pipeline;
    const auto& middlewares_to_append = GetMiddlewaresToAppend();

    resulting_pipeline.insert(resulting_pipeline.end(),
                              middlewares_to_append.begin(),
                              middlewares_to_append.end());

    return resulting_pipeline;
  }

  static yaml_config::Schema GetStaticConfigSchema();

 protected:
  const MiddlewaresList& GetMiddlewaresToAppend() const;

 private:
  MiddlewaresList middlewares_to_append_;
};

/// @ingroup userver_middlewares userver_base_classes
///
/// @brief Base class to build a per-handler middleware pipeline.
/// One may inherit from it and implement any custom logic, if desired.
/// By default the behavior is to use the server-wide pipeline.
class HandlerPipelineBuilder : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName{
      "default-handler-middleware-pipeline-builder"};

  HandlerPipelineBuilder(const components::ComponentConfig&,
                         const components::ComponentContext&);

  /// @brief The method to configure build a per-handler middleware pipeline,
  /// one may override it if custom behavior is desired.
  ///
  /// For example, a ping/ handler doesn't necessary need any business-logic
  /// related functionality, and could use just DefaultPipeline() for itself.
  ///
  /// @param server_middleware_pipeline the server-wide middleware pipeline
  virtual MiddlewaresList BuildPipeline(
      MiddlewaresList server_middleware_pipeline) const {
    return server_middleware_pipeline;
  }
};

}  // namespace server::middlewares

template <>
inline constexpr bool
    components::kHasValidate<server::middlewares::PipelineBuilder> = true;

template <>
inline constexpr auto
    components::kConfigFileMode<server::middlewares::PipelineBuilder> =
        ConfigFileMode::kNotRequired;

template <>
inline constexpr bool
    components::kHasValidate<server::middlewares::HandlerPipelineBuilder> =
        true;

template <>
inline constexpr auto
    components::kConfigFileMode<server::middlewares::HandlerPipelineBuilder> =
        ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
