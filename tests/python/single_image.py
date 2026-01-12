from pathlib import Path
from typing import Mapping
from types import MappingProxyType

from argolid import PyramidGenerartor
from .io_utils import ensure_repo_cloned, get_local_file_path, PolusTestDataRepo


DEFAULT_DOWNSAMPLE_METHODS: Mapping[int, str] = MappingProxyType({1: "mean"})


def generate_pyramid_from_repo_file(
    *,
    repo: PolusTestDataRepo,
    rel_image_path: Path,
    output_dir: str | Path,
    min_dim: int = 1024,
    vis_type: str = "Viv",
    downsample_methods: Mapping[int, str] = DEFAULT_DOWNSAMPLE_METHODS,
) -> None:
    """
    Clone the repo if needed, locate the image file, then run Argolid pyramid generation.
    """
    repo_dir = ensure_repo_cloned(repo)
    input_file = str(get_local_file_path(repo_dir, rel_image_path))

    output_dir = Path(output_dir)
    output_dir.parent.mkdir(parents=True, exist_ok=True)

    pyr_gen = PyramidGenerartor()
    pyr_gen.generate_from_single_image(
        input_file,
        str(output_dir),
        min_dim,
        vis_type,
        dict(downsample_methods),
    )


def main() -> None:
    repo = PolusTestDataRepo()

    rel_image_path = Path("argolid") / "x0_y0_c1.ome.tiff"
    output_dir = Path("output") / "one_image_ome_zarr"

    generate_pyramid_from_repo_file(
        repo=repo,
        rel_image_path=rel_image_path,
        output_dir=output_dir,
        min_dim=1024,
        vis_type="Viv",
        downsample_methods={1: "mean"},
    )


if __name__ == "__main__":
    main()
