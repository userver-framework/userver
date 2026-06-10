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
    OPTIMIZATION = "optimization"

MINOR_BUG_SIZE_THRESHOLD = 200
MINOR_SIZE_THRESHOLD = 50

class ClassifiedCommit(Commit):
    classification: Classification = Classification.UNCLEAR
    is_external: bool = False
    to_changelog: bool | None = None
    changelog_line: str | None = None
    commit_analysis: str | None = None
    component: str | None = None

def classify_commit(commit: Commit) -> Classification:
    doc_keywords = ["doc", "docs", "documentation", "readme"]
    commit_title_lower = commit.title.lower()
    has_docs_in_title = any(keyword in commit_title_lower for keyword in doc_keywords)
    
    fix_keywords = ["fix", "bugfix", "bug"]
    has_fix = any(keyword in commit_title_lower for keyword in fix_keywords)
    
    if has_docs_in_title:
        return Classification.DOCS
        
    if has_fix and commit.score_size <= MINOR_BUG_SIZE_THRESHOLD:
        return Classification.MINOR_BUG
        
    if commit.score_size <= MINOR_SIZE_THRESHOLD:
        return Classification.MINOR
        
    return Classification.UNCLEAR