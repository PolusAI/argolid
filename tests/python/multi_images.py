from pathlib import Path
from typing import Mapping
from types import MappingProxyType

from argolid import PyramidGenerartor
DEFAULT_DOWNSAMPLE_METHODS = MappingProxyType({1: "mean"})
from .io_utils import ensure_repo_cloned, PolusTestDataRepo


def generate_pyramid_from_repo_path(
    *,
    repo: PolusTestDataRepo,
    file_pattern: str,
    image_name: str = "test_image",
    output_dir: str | Path,
    min_dim: int = 1024,
    vis_type: str = "Viv",
    downsample_methods: Mapping[int, str] = DEFAULT_DOWNSAMPLE_METHODS,
) -> None:
    """
    Clone the repo if needed, locate the image file, then run Argolid pyramid generation.
    """
    repo_dir = ensure_repo_cloned(repo)
    input_dir = str(repo_dir / "argolid")

    output_dir = Path(output_dir)
    output_dir.parent.mkdir(parents=True, exist_ok=True)

    pyr_gen = PyramidGenerartor()
    pyr_gen.generate_from_image_collection(
        input_dir,
        file_pattern,
        image_name,
        str(output_dir),
        min_dim,
        vis_type,
        dict(downsample_methods),
    )


def main() -> None:
    repo = PolusTestDataRepo()
    file_pattern = "x{x:d}_y{y:d}_c{c:d}.ome.tiff"
    output_dir = Path("output") / "2D_pyramid_assembled"

    generate_pyramid_from_repo_path(
        repo=repo,
        file_pattern=file_pattern,
        image_name="test_image",
        output_dir=output_dir,
        min_dim=1024,
        vis_type="Viv",
        downsample_methods={1: "mean"},
    )


if __name__ == "__main__":
    main()
