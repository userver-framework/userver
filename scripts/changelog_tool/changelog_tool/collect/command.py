import pathlib
import re
from typing import List

import changelog_tool.common.git as git
from changelog_tool.collect.config import CollectConfig
from changelog_tool.collect.classification import Classification, classify_commit, ClassifiedCommit, should_send_to_llm

def collect(config: CollectConfig) -> None:
    print(f"Collecting commits from {config.from_sha} to {config.to_sha}...")
    commits: list[git.Commit] = git.get_commits(config.from_sha, config.to_sha, config.repo_path)
    
    core_team_regexes = [re.compile(pattern) for pattern in config.core_team_patterns]
    
    classified_commits: List[ClassifiedCommit] = []
    for commit in commits:
        is_core_team = any(regex.match(commit.author) for regex in core_team_regexes)
        classification = classify_commit(commit)
        classified_commits.append(ClassifiedCommit(
            **commit.model_dump(),
            classification=classification,
            is_external=not is_core_team,
            to_changelog=classification != Classification.UNCLEAR
        ))
        
    print(f"Found {len(classified_commits)} commits")