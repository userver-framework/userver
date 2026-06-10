import asyncio
import pathlib
import re
from typing import List, Dict, Any

import changelog_tool.common.git as git
import changelog_tool.common.io as io
from changelog_tool.config import Config
from changelog_tool.collect.classification import ClassifiedCommit, Classification
from changelog_tool.llm.client import HttpLLMClient
from changelog_tool.llm.processor import LLMProcessor
from changelog_tool.llm.config import LLMConfig


def report(config: Config) -> None:
    print(f"Loading classified commits from {config.report.output_dir}...")
    classified_commits: List[ClassifiedCommit] = io.load_classified_commits(
        config.report.output_dir, 'classified.json'
    )
    
    if not classified_commits:
        print("No classified commits found. Please run 'collect' command first.")
        return
    
    print(f"Found {len(classified_commits)} classified commits")
    
    # Load and apply overrides
    override_file = config.report.output_dir / 'override.yaml'
    if override_file.exists():
        print(f"Applying overrides from {override_file}...")
        _apply_overrides(classified_commits, override_file)
    
    # Identify commits that need LLM analysis
    commits_needing_analysis = [
        commit for commit in classified_commits
        if commit.to_changelog is True and (not commit.changelog_line or not commit.commit_analysis)
    ]
    
    if commits_needing_analysis:
        print(f"Found {len(commits_needing_analysis)} commits needing LLM analysis")
        
        # Create modified LLM config with 1.5x prompt size and truncate enabled
        modified_llm_config = LLMConfig(
            target_rps=config.llm_config.target_rps,
            retries=config.llm_config.retries,
            max_commits_per_batch=config.llm_config.max_commits_per_batch,
            max_user_prompt_length=int(config.llm_config.max_user_prompt_length * 1.5),
            include_diff=config.llm_config.include_diff,
            truncate_diff=True
        )
        
        llm_client = HttpLLMClient(modified_llm_config)
        llm_processor = LLMProcessor(modified_llm_config, llm_client, config.report.output_dir)
        
        llm_results = asyncio.run(llm_processor.process_commits(commits_needing_analysis))
        
        # Update commits with LLM results
        for commit in classified_commits:
            if commit.sha in llm_results:
                result = llm_results[commit.sha]
                commit.changelog_line = result.get("changelog_line", "")
                commit.commit_analysis = result.get("detailed_commit_analysis", "")
                try:
                    commit.classification = Classification(result.get("classification", "unclear"))
                except ValueError:
                    pass
                print(f"Updated commit {commit.sha} with LLM results")
    
    # Generate changelog
    print("Generating changelog...")
    changelog_content = _generate_changelog(classified_commits, config.report.github_url)
    
    # Save changelog
    changelog_file = config.report.output_dir / 'changelog.md'
    with open(changelog_file, 'w') as f:
        f.write(changelog_content)
    print(f"Generated changelog: {changelog_file}")


def _apply_overrides(commits: List[ClassifiedCommit], override_file: pathlib.Path) -> None:
    """Parse override.yaml and apply overrides to commits."""
    import yaml
    
    with open(override_file, 'r') as f:
        override_data = yaml.safe_load(f)
    
    if not override_data:
        return
    
    # Create a mapping of SHA to commit for quick lookup
    commit_map = {commit.sha: commit for commit in commits}
    
    for sha, override in override_data.items():
        if sha in commit_map:
            commit = commit_map[sha]
            if 'to_changelog' in override:
                commit.to_changelog = override['to_changelog']
            if 'changelog_line' in override:
                commit.changelog_line = override['changelog_line']
            if 'classification' in override:
                try:
                    commit.classification = Classification(override['classification'])
                except ValueError:
                    pass


def _generate_changelog(commits: List[ClassifiedCommit], github_url: str) -> str:
    """Generate formatted Markdown changelog."""
    lines = []
    
    # Group commits by classification
    groups: Dict[str, List[ClassifiedCommit]] = {}
    for commit in commits:
        if commit.to_changelog is True and commit.changelog_line:
            classification = commit.classification.value
            if classification not in groups:
                groups[classification] = []
            groups[classification].append(commit)
    
    # Define order of classifications
    classification_order = [
        "breaking-change",
        "feature",
        "optimization",
        "bug",
        "refactor",
        "minor",
        "docs",
        "unclear"
    ]
    
    # Generate sections for each classification
    for classification in classification_order:
        if classification not in groups:
            continue
        
        section_commits = groups[classification]
        if not section_commits:
            continue
        
        # Section header
        section_title = classification.replace("-", " ").title()
        lines.append(f"* {section_title}")
        lines.append("")
        
        # Group commits by component within each classification
        component_groups: Dict[str, List[ClassifiedCommit]] = {}
        commits_without_component = []
        
        for commit in section_commits:
            if commit.component:
                if commit.component not in component_groups:
                    component_groups[commit.component] = []
                component_groups[commit.component].append(commit)
            else:
                commits_without_component.append(commit)
        
        # Generate entries for each component
        for component in sorted(component_groups.keys()):
            component_commits = component_groups[component]
            lines.append(f"  * {component}")
            lines.append("")
            
            for commit in component_commits:
                short_sha = commit.sha[:8]
                line = f"    * {commit.changelog_line} <!-- {short_sha} -->"
                
                # Add external contributor thanks
                if commit.is_external:
                    author_name = _extract_author_name(commit.author)
                    line += f" Many thanks to {author_name} for the PR!"
                
                lines.append(line)
            
            lines.append("")
        
        # Generate entries for commits without component
        if commits_without_component:
            for commit in commits_without_component:
                short_sha = commit.sha[:8]
                line = f"  * {commit.changelog_line} <!-- {short_sha} -->"
                
                # Add external contributor thanks
                if commit.is_external:
                    author_name = _extract_author_name(commit.author)
                    line += f" Many thanks to {author_name} for the PR!"
                
                lines.append(line)
            
            lines.append("")
    
    # Collect external contributors not in changelog
    # Group by author and collect their commit titles
    external_contributors_not_in_changelog: Dict[str, List[str]] = {}
    for commit in commits:
        if commit.is_external and (commit.to_changelog is False or commit.to_changelog is None):
            author_name = _extract_author_name(commit.author)
            if author_name not in external_contributors_not_in_changelog:
                external_contributors_not_in_changelog[author_name] = []
            external_contributors_not_in_changelog[author_name].append(commit.title)
    
    if external_contributors_not_in_changelog:
        lines.append("* Many thanks to:")
        for contributor in sorted(external_contributors_not_in_changelog.keys()):
            titles = external_contributors_not_in_changelog[contributor]
            if len(titles) == 1:
                lines.append(f"  * {contributor} for {titles[0]}!")
            else:
                lines.append(f"  * {contributor} for:")
                for title in titles:
                    lines.append(f"    * {title}")
        lines.append("")
    
    return "\n".join(lines)


def _extract_author_name(author: str) -> str:
    """Extract author name from 'Name <email>' format."""
    match = re.match(r'^(.+?)\s*<', author)
    if match:
        return match.group(1).strip()
    return author
