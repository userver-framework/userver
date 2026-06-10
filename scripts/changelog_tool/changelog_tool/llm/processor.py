import asyncio
import json
from typing import List, Dict, Any
from pathlib import Path

from changelog_tool.common.git import Commit, get_commit_diff
from changelog_tool.llm.client import BaseLLMClient
from changelog_tool.llm.config import LLMConfig
from changelog_tool.llm.state import LLMState
from changelog_tool.llm.exceptions import LLMError, LLMTransientError

class LLMProcessor:
    def __init__(self, config: LLMConfig, llm_client: BaseLLMClient, output_dir: Path):
        self.config = config
        self.llm_client = llm_client
        self.output_dir = output_dir
        self.state = LLMState(output_dir / "llm_state.json")
        
    async def process_commits(self, commits: List[Commit]) -> Dict[str, Dict[str, Any]]:
        """
        Asynchronously processes a list of commits through the LLM.
        Returns a dictionary SHA -> dict with results (classification, changelog_line, detailed_commit_analysis).
        """
        # Load and clean state
        await self.state.load()
        valid_shas = {commit.sha for commit in commits}
        await self.state.cleanup(valid_shas)
        
        # Filter commits for processing
        commits_to_process = []
        results = {}
        
        for commit in commits:
            # Check state
            result = await self.state.get_result(commit.sha)
            if result:
                results[commit.sha] = result
            else:
                commits_to_process.append(commit)
                
        print(f"Found {len(commits)} commits, {len(results)} already processed, {len(commits_to_process)} to process via LLM")
        
        if not commits_to_process:
            return results
            
        # Split into batches considering prompt size
        batches = self._create_smart_batches(commits_to_process)
        
        total_commits = sum(len(batch) for batch in batches)
        print(f"Processing {total_commits} commits in {len(batches)} batches...")
        
        # Process batches in parallel
        batch_results = await asyncio.gather(
            *[self._process_batch(batch, i, len(batches), total_commits) for i, batch in enumerate(batches)],
            return_exceptions=True
        )
        
        # Collect results
        completed_batches = 0
        for batch_idx, batch_result in enumerate(batch_results):
            if isinstance(batch_result, Exception):
                # Batch errors are already written to state, just continue
                continue
            completed_batches += 1
            results.update(batch_result)
        
        print(f"Completed {completed_batches}/{len(batches)} batches")
            
        return results
        
    async def _process_batch(self, batch: List[Commit], batch_idx: int = 0, total_batches: int = 0, total_commits: int = 0) -> Dict[str, Dict[str, Any]]:
        """Processes one batch of commits."""
        try:
            print(f"[{batch_idx + 1}/{total_batches}] Processing {len(batch)} commits...")
            prompt = self._build_prompt(batch)
            
            # Check prompt length
            if len(prompt) > self.config.max_user_prompt_length:
                if self.config.truncate_diff:
                    prompt = self._truncate_prompt(prompt)
                else:
                    # Mark all batch commits as erroneous
                    error_msg = f"Prompt too long ({len(prompt)} > {self.config.max_user_prompt_length})"
                    for commit in batch:
                        await self.state.set_error(commit.sha, error_msg)
                    return {
                        commit.sha: {
                            "classification": "unclear",
                            "to_changelog": False,
                            "changelog_line": "",
                            "detailed_commit_analysis": ""
                        } for commit in batch
                    }
                    
            # Send to LLM
            response_text = await self.llm_client.generate(prompt)
            
            # Remove markdown code blocks if present
            if response_text.strip().startswith('```json'):
                response_text = response_text.strip()[7:]  # Remove ```json
            if response_text.strip().startswith('```'):
                response_text = response_text.strip()[3:]  # Remove ```
            if response_text.strip().endswith('```'):
                response_text = response_text.strip()[:-3]  # Remove trailing ```
            response_text = response_text.strip()
            
            # Parse response
            try:
                response_data = json.loads(response_text)
            except json.JSONDecodeError as e:
                raise LLMError(f"LLM returned invalid JSON: {e}")
                
            # Check response format
            if not isinstance(response_data, dict):
                raise LLMError("LLM returned invalid response format (not a dict)")
                
            # Save results and return
            results = {}
            for commit in batch:
                commit_data = response_data.get(commit.sha, {})
                if isinstance(commit_data, str):
                    # Fallback if LLM returned just a string
                    classification = commit_data
                    to_changelog = classification in ["feature", "breaking-change"]
                    changelog_line = ""
                    detailed_commit_analysis = ""
                else:
                    classification = commit_data.get("classification", "unclear")
                    to_changelog = commit_data.get("to_changelog", None)
                    changelog_line = commit_data.get("changelog_line", "")
                    detailed_commit_analysis = commit_data.get("detailed_commit_analysis", "")
                
                completed = await self.state.set_result(commit.sha, classification, changelog_line, detailed_commit_analysis, to_changelog)
                if total_commits > 0:
                    remaining = total_commits - completed
                    print(f"  Progress: {completed}/{total_commits} commits, {remaining} remaining")
                
                results[commit.sha] = {
                    "classification": classification,
                    "to_changelog": to_changelog,
                    "changelog_line": changelog_line,
                    "detailed_commit_analysis": detailed_commit_analysis
                }
                
            print(f"[{batch_idx + 1}/{total_batches}] ✓ Completed")
            return results
            
        except LLMError:
            # Critical error - re-raise
            raise
        except Exception as e:
            # Temporary error or other problem - mark commits as erroneous
            error_msg = f"{type(e).__name__}: {str(e)}"
            print(f"✗ Batch {batch_idx + 1}/{total_batches} failed: {error_msg}")
            for commit in batch:
                await self.state.set_error(commit.sha, error_msg)
            return {
                commit.sha: {
                    "classification": "unclear",
                    "to_changelog": None,
                    "changelog_line": "",
                    "detailed_commit_analysis": ""
                } for commit in batch
            }
            
    def _create_smart_batches(self, commits: List[Commit]) -> List[List[Commit]]:
        if not commits:
            return []
        
        batches = []
        current_batch = []
        current_prompt_size = 0
        
        system_prompt_size = len(self._build_prompt([]))
        
        for commit in commits:
            commit_prompt_size = self._estimate_commit_size(commit)
            
            can_add = (
                len(current_batch) < self.config.max_commits_per_batch and
                (current_prompt_size + commit_prompt_size + system_prompt_size) <= self.config.max_user_prompt_length
            )
            
            if can_add:
                current_batch.append(commit)
                current_prompt_size += commit_prompt_size
            else:
                if current_batch:
                    batches.append(current_batch)
                
                current_batch = [commit]
                current_prompt_size = commit_prompt_size
        
        if current_batch:
            batches.append(current_batch)
        
        return batches
    
    def _estimate_commit_size(self, commit: Commit) -> int:
        """Estimates the prompt size for one commit."""
        size = len(commit.sha) + len(commit.title) + len(commit.message)
        size += len(', '.join(f.path for f in commit.changed_files))
        
        if self.config.include_diff:
            diff = get_commit_diff(commit)
            size += len(diff)
        
        return size + 200  # reserve for JSON formatting and separators
    
    def _build_prompt(self, commits: List[Commit]) -> str:
        """Forms a prompt for a batch of commits."""
        system_prompt = """You are an expert software engineer analyzing git commits for a changelog.
Your task is to analyze commits since the last release and highlight important and interesting changes.
Ignore simple bugfixes, typos, and minor refactoring.

IMPORTANT: This is for the USERVER project - a C++ asynchronous framework. Focus on changes that are significant for users of this framework.

For each commit, you MUST provide a JSON object with the following fields:
1. "classification": One of ["feature", "breaking-change", "refactor", "minor", "optimization", "unclear"].
   - Use "breaking-change" if the commit introduces backward-incompatible changes.
   - Use "feature" for new functionality that is important for USERVER users.
   - Use "refactor" for significant architectural changes.
   - Use "minor" for small improvements.
   - Use "optimization" for performance improvements, optimizations, and efficiency gains.
   - Use "unclear" if you cannot determine the classification.
2. "to_changelog": Boolean - MUST be true for:
   - ALL breaking-change commits (these are critical for users)
   - Features that are significant for USERVER users (new components, major APIs, important functionality)
   - MUST be false for: minor refactoring, bugfixes, typos, internal changes, test updates
3. "changelog_line": A concise, user-friendly description of the change suitable for a changelog.
   - IMPORTANT: If the classification is "breaking-change", you MUST include migration or fix instructions in this line if they are present in the commit message.
   - Only include this if to_changelog is true.
4. "detailed_commit_analysis": A detailed analysis of what was added, why it was added, and what impact or benefit it brings to the project.

You MUST return a valid JSON object where keys are commit SHAs and values are the analysis objects.
Example output format:
{
  "commit_sha_1": {
    "classification": "feature",
    "to_changelog": true,
    "changelog_line": "Added support for async LLM processing",
    "detailed_commit_analysis": "Added a new LLMProcessor class to handle batching and async requests. This improves performance by allowing parallel processing of commits."
  },
  "commit_sha_2": {
    "classification": "breaking-change",
    "to_changelog": true,
    "changelog_line": "Changed config format. Migration: rename 'llm_config' to 'llm-config' in your yaml file.",
    "detailed_commit_analysis": "Updated the configuration schema to use hyphens instead of underscores for consistency. This breaks existing configs but aligns with the project's naming conventions."
  },
  "commit_sha_3": {
    "classification": "minor",
    "to_changelog": false,
    "changelog_line": "",
    "detailed_commit_analysis": "Fixed typo in documentation."
  }
}
"""
        
        user_parts = []
        for commit in commits:
            part = f"Commit SHA: {commit.sha}\n"
            part += f"Title: {commit.title}\n"
            part += f"Message: {commit.message}\n"
            part += f"Changed Files: {', '.join(f.path for f in commit.changed_files)}\n"
            
            if self.config.include_diff:
                diff = get_commit_diff(commit)
                part += f"Diff:\n{diff}\n"
                
            user_parts.append(part)
            
        user_prompt = "Please analyze the following commits:\n\n" + "\n---\n".join(user_parts)
        return f"{system_prompt}\n\n{user_prompt}"
        
    def _truncate_prompt(self, prompt: str) -> str:
        """Truncates the prompt to the allowed length."""
        # Simple truncation - in reality, smarter logic may be needed
        if len(prompt) <= self.config.max_user_prompt_length:
            return prompt
        return prompt[:self.config.max_user_prompt_length]