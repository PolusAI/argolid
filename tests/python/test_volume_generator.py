from pathlib import Path

import pytest

from argolid import VolumeGenerator


@pytest.mark.parametrize(
    "rel_input_dir,file_pattern,output_dir,image_name,group_by",
    [
        ("3D", "{z:d}.c{c:d}.ome.tiff", "3D_pyramid_full", "test_volume", "c"),
    ],
)
def test_generate_volume_creates_output(
    tmp_path: Path,
    rel_input_dir: str,
    file_pattern: str,
    output_dir: str,
    image_name: str,
    group_by: str,
) -> None:
    # Resolve input directory relative to THIS test file
    tests_dir = Path(__file__).resolve().parent
    input_dir = tests_dir / rel_input_dir
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
