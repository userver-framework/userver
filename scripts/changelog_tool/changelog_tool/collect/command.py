import pathlib

import changelog_tool.common.git as git

def collect(from_sha: str, to_sha: str, repo_path: pathlib.Path) -> None:
    print(f"Collecting commits from {from_sha} to {to_sha}...")
    commits: list[git.Commit] = git.get_commits(from_sha, to_sha, repo_path)
    
    print(f"Found {len(commits)} commits")