import json
import pathlib
from typing import List

from changelog_tool.collect.classification import ClassifiedCommit


def dump_classified_commits(commits: List[ClassifiedCommit], output_dir: pathlib.Path, filename: str) -> None:
    # Ensure output directory exists
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Create full path to output file
    output_file = output_dir / filename
    
    # Convert classified commits to JSON format
    json_data = [commit.model_dump() for commit in commits]
    json_str = json.dumps(json_data, indent=2)
    
    # Write to file
    with open(output_file, 'w') as f:
        f.write(json_str)


def load_classified_commits(output_dir: pathlib.Path, filename: str) -> List[ClassifiedCommit]:
    # Create full path to input file
    input_file = output_dir / filename
    
    # Check if file exists
    if not input_file.exists():
        return []
    
    # Read from file
    with open(input_file, 'r') as f:
        json_data = json.load(f)
    
    # Convert JSON data to ClassifiedCommit objects
    return [ClassifiedCommit(**item) for item in json_data]