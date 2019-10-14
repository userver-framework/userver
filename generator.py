import argparse
import os
import sys
from typing import List
from typing import Optional

CODEGEN_SUBMODULE_PATH = os.path.join(
    os.path.dirname(__file__), 'submodules', 'codegen',
)

sys.path.extend([CODEGEN_SUBMODULE_PATH])

# pylint: disable=wrong-import-position
from codegen import plugin_manager  # noqa: E402

# TODO(oyaso): move common part to submodule


class Repository:
    config_filename: str = 'services.yaml'
    root_dir: str

    def __init__(self, root_path: str, argv: Optional[List[str]]) -> None:
        self.root_dir = os.path.abspath(root_path)

        parser = argparse.ArgumentParser()
        parser.add_argument('--build-dir', required=True)
        parser.add_argument('--log-level', default='WARNING')
        self.args = parser.parse_args(argv)

        plugin_manager.init_logger(self.args.log_level)
        self.build_dir = self.args.build_dir

    def register_plugins(self, manager: plugin_manager.ScopeManager) -> None:
        manager.add_plugins_path(os.path.join(self.root_dir, 'plugins'))

        manager.add_params(
            root_dir=self.root_dir,
            root_build_dir=self.build_dir,
        )

    def __repr__(self) -> str:
        return '<Repository %s>' % self.root_dir


def main(argv=None):
    repo_dir = os.path.dirname(__file__)
    repo = Repository(repo_dir, argv)
    manager = plugin_manager.PluginManager(repo, repo_dir)
    manager.init()
    manager.configure()
    manager.generate()


if __name__ == '__main__':
    main()
