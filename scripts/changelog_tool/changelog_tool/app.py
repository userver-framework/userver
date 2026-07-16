"""Composition root: constructs and wires all application objects."""

from __future__ import annotations

from changelog_tool.config import Config
from changelog_tool.llm.client import HttpLLMClient
from changelog_tool.llm.processor import LLMProcessor
from changelog_tool.pipeline import OutputRenderer
from changelog_tool.pipeline import ResumeLoop


class AppContainer:
    """Wires the full dependency graph for a single invocation."""

    def __init__(self, config: Config) -> None:
        self.config = config

        self.llm_client = HttpLLMClient(config.llm_config)
        self.processor = LLMProcessor(
            config.llm_config,
            self.llm_client,
            config.generate.output_dir,
            config.generate.repo_path,
        )

        self.resume_loop = ResumeLoop(config)
        self.output_renderer = OutputRenderer(config)
