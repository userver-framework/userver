import os
import sys

# pylint: disable=import-error
from codegen import plugin_manager  # noqa: I100, I201


sys.path.append(os.path.join(os.path.dirname(__file__), 'impl'))

# pylint: disable=wrong-import-position
import cmake_generator  # noqa: I100, I201


class RepositoryGenerator:
    def __init__(self, config: dict) -> None:
        self.dependencies: dict = {}

    def configure(self, manager: plugin_manager.ConfigureManager) -> None:
        dependencies_dir = os.path.join(
            manager.params.root_dir, 'external-deps',
        )
        self.dependencies = cmake_generator.parse_deps_files(dependencies_dir)

    def generate(self, manager: plugin_manager.GenerateManager) -> None:
        if not self.dependencies:
            return

        cmake_generated_path = os.path.join(
            manager.params.root_build_dir, 'cmake_generated',
        )
        os.makedirs(cmake_generated_path, exist_ok=True)

        for key, value in self.dependencies.items():
            for filename, data in cmake_generator.generate_cmake(
                    key, value, manager.renderer,
            ).items():
                manager.write(
                    os.path.join(cmake_generated_path, filename), data,
                )
