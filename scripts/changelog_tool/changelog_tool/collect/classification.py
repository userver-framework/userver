from enum import Enum
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from changelog_tool.common.git import Commit


class Classification(str, Enum):
    FEATURE = "feature"
    BUG = "bug"
    BREAKING_CHANGE = "breaking-change"
    MINOR_BUG = "minor_bug"
    REFACTOR = "refactor"
    DOCS = "docs"
    UNCLEAR = "unclear"


class ClassifiedCommit(Commit):
    classification: Classification = Classification.UNCLEAR
    is_external: bool = False
    to_changelog: bool | None = None

def classify_commit(commit: "Commit") -> Classification:
    # Default to unclear if no heuristics match
    return Classification.UNCLEAR