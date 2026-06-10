import re
import asyncio
import os
from typing import List

import changelog_tool.common.git as git
import changelog_tool.common.io as io
from changelog_tool.config import Config
from changelog_tool.collect.classification import Classification, classify_commit, ClassifiedCommit
from changelog_tool.llm.client import HttpLLMClient
from changelog_tool.llm.processor import LLMProcessor
from changelog_tool.llm.exceptions import LLMError


def _extract_component_from_title(title: str) -> str | None:
    """Extract component name from commit title.
    
    Examples:
    - "feat odbc: improve driver" -> "odbc"
    - "fix(redis): connection leak" -> "redis"
    - "feat chaotic: deal with..." -> "chaotic"
    - "docs: update README" -> None
    """
    # Pattern: type(component): or type component: or type component description
    match = re.match(r'^(\w+)(?:\(([^)]+)\))?:?\s*(.+)', title)
    if match:
        commit_type = match.group(1)
        component = match.group(2)
        description = match.group(3)
        
        # If component in parentheses, use it
        if component:
            return component.lower()
        
        # If no component in parentheses, check description
        words = description.split()
        if words:
            # Check if first word ends with colon (e.g., "odbc: improve driver")
            if words[0].endswith(':'):
                return words[0][:-1].lower()
            # Check if first word is followed by a colon (e.g., "chaotic: deal with...")
            if len(words) > 1 and words[1].startswith(':'):
                return words[0].lower()
    return None

def collect(config: Config) -> None:
    print(f"Collecting commits from {config.collect.from_sha} to {config.collect.to_sha}...")
    commits: list[git.Commit] = git.get_commits(config.collect.from_sha, config.collect.to_sha, config.collect.repo_path)
    
    core_team_regexes = [re.compile(pattern) for pattern in config.collect.core_team_patterns]
    
    classified_commits: List[ClassifiedCommit] = []
    for commit in commits:
        is_core_team = any(regex.match(commit.author) for regex in core_team_regexes)
        classification = classify_commit(commit)
        # Extract component from title (e.g., "feat odbc: improve driver" -> "odbc")
        component = _extract_component_from_title(commit.title)
        
        classified_commit = ClassifiedCommit(
            **commit.model_dump(),
            classification=classification,
            is_external=not is_core_team,
            to_changelog=None,
            changelog_line=None,
            commit_analysis=None,
            component=component
        )
        
        if classification in [Classification.FEATURE, Classification.BUG, Classification.BREAKING_CHANGE]:
            raise RuntimeError("Unexpected positive changelog preclassification")
        elif classification == Classification.UNCLEAR:
            classified_commit.to_changelog = None
        else:
            classified_commit.to_changelog = False

        classified_commits.append(classified_commit)
        
    print(f"Found {len(classified_commits)} commits")

    io.dump_classified_commits(classified_commits, config.collect.output_dir, 'preclassified.json')
    
    llm_client = HttpLLMClient(config.llm_config)
    llm_processor = LLMProcessor(config.llm_config, llm_client, config.collect.output_dir)
            
    unclear_commits = [
        commit for commit in classified_commits
        if commit.classification == Classification.UNCLEAR
    ]
            
    llm_results = asyncio.run(llm_processor.process_commits(unclear_commits))
            
    for commit in classified_commits:
        if commit.sha in llm_results:
            result = llm_results[commit.sha]
            try:
                commit.classification = Classification(result.get("classification", "unclear"))
            except ValueError:
                # If LLM returned an unknown classification, keep UNCLEAR
                pass
            
            commit.to_changelog = result.get("to_changelog")
            commit.changelog_line = result.get("changelog_line")
            commit.commit_analysis = result.get("detailed_commit_analysis")
                        

    io.dump_classified_commits(classified_commits, config.collect.output_dir, 'classified.json')