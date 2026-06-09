import pathlib
import re

import changelog_tool.common.git as git
from changelog_tool.collect.config import CollectConfig

def collect(config: CollectConfig) -> None:
    print(f"Collecting commits from {config.from_sha} to {config.to_sha}...")
    commits: list[git.Commit] = git.get_commits(config.from_sha, config.to_sha, config.repo_path)
    
    core_team_regexes = [re.compile(pattern) for pattern in config.core_team_patterns]
    
    for commit in commits:
        is_core_team = any(regex.match(commit.author) for regex in core_team_regexes)
        commit.is_external = not is_core_team
    
    print(f"Found {len(commits)} commits")