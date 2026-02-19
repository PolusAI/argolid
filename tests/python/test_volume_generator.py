from pathlib import Path

import pytest

from argolid import VolumeGenerator
from .io_utils import ensure_repo_cloned, PolusTestDataRepo


@pytest.mark.parametrize(
    "file_pattern,output_dir,image_name,group_by",
    [
        ("{z:d}.c{c:d}.ome.tiff", "volume_full", "test_volume", "c"),
    ],
)
def test_generate_volume_creates_output(
    tmp_path: Path,
    file_pattern: str,
    output_dir: str,
    image_name: str,
    group_by: str,
) -> None:

    repo = PolusTestDataRepo()
    repo_dir = ensure_repo_cloned(repo)

    input_dir = repo_dir / "argolid" / "3D"
    assert input_dir.is_dir(), f"Missing test data dir: {input_dir}"

    out_dir = tmp_path / output_dir

    vg = VolumeGenerator(
        source_dir=str(input_dir),
        group_by=group_by,
        file_pattern=file_pattern,
        out_dir=str(out_dir),
        image_name=image_name,
    )

    vg.generate_volume()

    root = out_dir / image_name
    assert root.is_dir(), f"Expected output root dir: {root}"

    base_scale = root / "0"
    assert base_scale.is_dir(), f"Expected base scale dir: {base_scale}"

    # zarr markers
    zarr_markers = [
        root / ".zgroup",
        root / ".zattrs",
        base_scale / ".zgroup",
        base_scale / ".zattrs",
    ]
    assert any(p.exists() for p in zarr_markers), (
        "Expected at least one zarr metadata marker (.zgroup/.zattrs) "
        f"under {root} or {base_scale}"
    )
