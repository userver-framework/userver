#pragma once

/// @file engine/task/task_processor_fwd.hpp
/// @brief @copybrief engine::TaskProcessor

/// Asynchronous engine primitives
namespace engine {

/// @brief Manages tasks execution on OS threads.
///
/// To create a task processor add its configuration to the "task_processors"
/// section of the components::ManagerControllerComponent static configuration.
class TaskProcessor;

}  // namespace engine
