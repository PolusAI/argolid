import shutil
from pathlib import Path

import pytest

from .io_utils import PolusTestDataRepo
from .multi_images import generate_pyramid_from_repo_path
from .zarr_assertions import assert_is_argolid_omexml_zarr_pyramid


@pytest.mark.integration
def test_stitched_image_collection_pyramid(tmp_path: Path) -> None:
    if shutil.which("git") is None:
        pytest.skip("git not available")

    repo = PolusTestDataRepo(repo_dir=tmp_path / "polus-test-data")
    out_dir = tmp_path / "out"

    generate_pyramid_from_repo_path(
        repo=repo,
        file_pattern="x{x:d}_y{y:d}_c{c:d}.ome.tiff",
        image_name="stitched_image",
        output_dir=out_dir,
        min_dim=1024,
        vis_type="Viv",
        downsample_methods={1: "mean"},
    )

    shapes = assert_is_argolid_omexml_zarr_pyramid(out_dir / "stitched_image.zarr" , expect_levels=2)

    # ensure at least one spatial dimension shrinks between level 0 and 1
    y0, x0 = shapes[0][-2], shapes[0][-1]
    y1, x1 = shapes[1][-2], shapes[1][-1]
    assert (y1 < y0) or (x1 < x0), f"Expected level 1 to be downsampled vs level 0, got L0={shapes[0]} L1={shapes[1]}"
