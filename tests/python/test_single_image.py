import shutil
from pathlib import Path

import pytest

from .io_utils import PolusTestDataRepo
from .single_image import generate_pyramid_from_repo_file
from .zarr_assertions import assert_is_argolid_omexml_zarr_pyramid


@pytest.mark.integration
def test_single_image_pyramid(tmp_path: Path) -> None:
    if shutil.which("git") is None:
        pytest.skip("git not available")

    repo = PolusTestDataRepo(repo_dir=tmp_path / "polus-test-data")
    out_dir = tmp_path / "out" / "single_image"

    generate_pyramid_from_repo_file(
        repo=repo,
        rel_image_path=Path("argolid") / "2D" / "x0_y0_c1.ome.tiff",
        output_dir=out_dir,
    )

    shapes = assert_is_argolid_omexml_zarr_pyramid(out_dir, expect_levels=2)

    # ensure at least one spatial dimension shrinks between level 0 and 1
    y0, x0 = shapes[0][-2], shapes[0][-1]
    y1, x1 = shapes[1][-2], shapes[1][-1]
    assert (y1 < y0) or (x1 < x0), f"Expected level 1 to be downsampled vs level 0, got L0={shapes[0]} L1={shapes[1]}"
