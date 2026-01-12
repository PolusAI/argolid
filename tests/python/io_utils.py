from dataclasses import dataclass
from pathlib import Path
import subprocess



@dataclass(frozen=True)
class PolusTestDataRepo:
    repo_url: str = "https://github.com/sameeul/polus-test-data.git"
    repo_dir: Path = Path("polus-test-data")
    default_branch: str = "main"


def ensure_repo_cloned(repo: PolusTestDataRepo, depth: int = 1) -> Path:
    """
    Ensure the repo exists locally. If not, clone it.
    Returns the local repo directory.
    """
    if repo.repo_dir.exists():
        return repo.repo_dir

    repo.repo_dir.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        ["git", "clone", "--depth", str(depth), repo.repo_url, str(repo.repo_dir)],
        check=True,
    )
    return repo.repo_dir


def get_local_file_path(repo_dir: Path, rel_path: Path) -> Path:
    """
    Resolve a path inside the repo and validate it exists.
    """
    local_path = (repo_dir / rel_path).resolve()
    if not local_path.exists():
        raise FileNotFoundError(f"File not found: {local_path}")
    return local_path