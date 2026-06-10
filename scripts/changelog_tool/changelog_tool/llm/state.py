import json
import asyncio
from pathlib import Path
from typing import Dict, Any, Set, Optional

class LLMState:
    def __init__(self, state_file_path: Path):
        self.state_file_path = state_file_path
        self.state: Dict[str, Dict[str, Any]] = {}
        self.lock = asyncio.Lock()
        
    async def load(self) -> None:
        """Асинхронно загружает состояние из файла."""
        async with self.lock:
            if self.state_file_path.exists():
                try:
                    with open(self.state_file_path, 'r', encoding='utf-8') as f:
                        loaded_state = json.load(f)
                        # Убедимся, что состояние имеет правильный формат
                        if isinstance(loaded_state, dict):
                            self.state = loaded_state
                        else:
                            self.state = {}
                except (json.JSONDecodeError, IOError) as e:
                    print(f"Warning: Could not load state file {self.state_file_path}: {e}")
                    self.state = {}
            else:
                self.state = {}
                
    async def save(self) -> None:
        """Асинхронно сохраняет состояние в файл."""
        # Создаем директорию если её нет
        self.state_file_path.parent.mkdir(parents=True, exist_ok=True)
        
        # Атомарная запись через временный файл
        temp_file = self.state_file_path.with_suffix('.tmp')
        try:
            with open(temp_file, 'w', encoding='utf-8') as f:
                json.dump(self.state, f, ensure_ascii=False, indent=2)
            temp_file.replace(self.state_file_path)
        except IOError as e:
            print(f"Error: Could not save state file {self.state_file_path}: {e}")
            if temp_file.exists():
                temp_file.unlink()
                    
    async def cleanup(self, valid_shas: Set[str]) -> None:
        """Удаляет из стейта коммиты, не попавшие в текущую выборку."""
        async with self.lock:
            keys_to_remove = set(self.state.keys()) - valid_shas
            for key in keys_to_remove:
                del self.state[key]
            if keys_to_remove:
                await self.save()
                
    async def get_result(self, sha: str) -> Optional[Dict[str, Any]]:
        """Возвращает результат анализа коммита, если он есть и не содержит ошибки."""
        async with self.lock:
            commit_data = self.state.get(sha)
            if commit_data and commit_data.get("error") is None:
                return commit_data
            return None
            
    async def set_result(self, sha: str, classification: str, changelog_line: str, detailed_commit_analysis: str, to_changelog: bool = False) -> int:
        """Сохраняет успешный результат классификации. Возвращает количество готовых коммитов."""
        async with self.lock:
            self.state[sha] = {
                "classification": classification,
                "to_changelog": to_changelog,
                "changelog_line": changelog_line,
                "detailed_commit_analysis": detailed_commit_analysis,
                "error": None
            }
            completed = len([k for k, v in self.state.items() if v.get("error") is None])
            await self.save()
            return completed
            
    async def set_error(self, sha: str, error_message: str) -> None:
        """Сохраняет ошибку классификации."""
        async with self.lock:
            self.state[sha] = {
                "classification": "unclear",
                "error": error_message
            }
            await self.save()