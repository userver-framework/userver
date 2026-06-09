from enum import Enum

from changelog_tool.common.git import Commit


class Classification(str, Enum):
    FEATURE = "feature"
    BUG = "bug"
    BREAKING_CHANGE = "breaking-change"
    MINOR_BUG = "minor_bug"
    REFACTOR = "refactor"
    DOCS = "docs"
    UNCLEAR = "unclear"
    MINOR="minor"

MINOR_BUG_SIZE_THRESHOLD = 200
MINOR_SIZE_THRESHOLD = 50

class ClassifiedCommit(Commit):
    classification: Classification = Classification.UNCLEAR
    is_external: bool = False
    to_changelog: bool | None = None

def classify_commit(commit: Commit) -> Classification:
    has_docs_in_files = any(
        "docs/" in file_change.path.lower() or
        "documentation" in file_change.path.lower()
        for file_change in commit.changed_files
    )
    
    doc_keywords = ["doc", "docs", "documentation", "readme"]
    commit_title_lower = commit.title.lower()
    has_docs_in_title = any(keyword in commit_title_lower for keyword in doc_keywords)
    
    fix_keywords = ["fix", "bugfix", "bug"]
    has_fix = any(keyword in commit_title_lower for keyword in fix_keywords)
    
    if has_docs_in_files or has_docs_in_title:
        return Classification.DOCS
        
    if has_fix and commit.score_size <= MINOR_BUG_SIZE_THRESHOLD:
        return Classification.MINOR_BUG
        
    if commit.score_size <= MINOR_SIZE_THRESHOLD:
        return Classification.MINOR
        
    return Classification.UNCLEAR