"""Shared data models for classified commits."""

from enum import Enum

from changelog_tool.common.git import Commit


class Classification(str, Enum):
    BREAKING_CHANGE = 'breaking-change'
    FEATURE = 'feature'
    OPTIMIZATION = 'optimization'
    REFACTOR = 'refactor'
    MINOR = 'minor'
    DOCS = 'docs'
    UNCLEAR = 'unclear'


class ExternalGroup(str, Enum):
    """Bottom-section bucket assigned by the creative external-contribution pass."""

    BUILD = 'Build'
    DOCUMENTATION = 'Documentation'
    FIXES = 'Fixes'
    OTHER = 'Other'


class ClassifiedCommit(Commit):
    classification: Classification = Classification.UNCLEAR
    is_external: bool = False
    to_changelog: bool | None = None
    changelog_line: str | None = None
    commit_analysis: str | None = None
    component: str | None = None
    needs_attention: bool = False
    migration_guide: str | None = None
    external_group: ExternalGroup | None = None
    github_login: str | None = None
    github_name: str | None = None
    github_profile_url: str | None = None
