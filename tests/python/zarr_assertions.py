from __future__ import annotations

from pathlib import Path
import zarr


def find_argolid_root(out_dir: Path) -> Path:
    """
    Find the Argolid output root directory under out_dir.
    The root is identified by:
      - directory ending with .zarr or .ome.zarr
      - containing METADATA.ome.xml
      - containing data.zarr/
    """
    out_dir = out_dir.resolve()

    if not out_dir.exists():
        raise FileNotFoundError(f"Output directory does not exist: {out_dir}")

    # If caller passed the root itself
    if (
        out_dir.is_dir()
        and out_dir.suffix == ".zarr"
        and (out_dir / "METADATA.ome.xml").exists()
        and (out_dir / "data.zarr").is_dir()
    ):
        return out_dir

    # Otherwise search one level down
    candidates = []
    for p in out_dir.iterdir():
        if not p.is_dir():
            continue
        if p.suffix != ".zarr":
            continue
        if not (p / "METADATA.ome.xml").exists():
            continue
        if not (p / "data.zarr").is_dir():
            continue
        candidates.append(p)

    if len(candidates) == 1:
        return candidates[0]

    if len(candidates) > 1:
        return max(candidates, key=lambda p: p.stat().st_mtime)

    raise FileNotFoundError(
        f"No Argolid zarr output found under {out_dir}. "
        f"Expected a *.zarr directory containing METADATA.ome.xml and data.zarr/"
    )


def open_argolid_data_group(argolid_root: Path) -> zarr.Group:
    """
    Open the data.zarr group inside Argolid output.
    """
    data_path = argolid_root / "data.zarr"
    if not data_path.exists():
        raise FileNotFoundError(f"Missing data.zarr at {data_path}")
    return zarr.open_group(str(data_path), mode="r")


def assert_is_argolid_omexml_zarr_pyramid(out_dir: Path, expect_levels: int | None = None) -> list[tuple[int, ...]]:
    """
    Validate Argolid-style output:
      <name>.ome.zarr/METADATA.ome.xml
      <name>.ome.zarr/data.zarr/0/<level>/...

    Returns:
      Shapes for each pyramid level found under data.zarr/0/
    """
    root = find_argolid_root(out_dir)

    # METADATA.ome.xml exists and non-empty
    ome_xml = root / "METADATA.ome.xml"
    assert ome_xml.exists(), f"Missing {ome_xml}"
    assert ome_xml.stat().st_size > 0, f"Empty metadata file: {ome_xml}"

    data = open_argolid_data_group(root)

    # Argolid stores pyramids under "0/<level>"
    assert "0" in data, f"Expected '0' in data.zarr. keys={list(data.keys())}"
    series0 = data["0"]
    assert isinstance(series0, zarr.Group)

    # levels can be arrays or groups
    level_names = sorted(
        [k for k in series0.keys() if str(k).isdigit()],
        key=lambda s: int(s),
    )

    assert level_names, (
        f"No pyramid levels found under data.zarr/0. "
        f"keys={list(series0.keys())}, groups={list(series0.group_keys())}, arrays={list(series0.array_keys())}"
    )

    level_strs = {str(k) for k in level_names}
    assert "0" in level_strs, f"Missing level 0. Levels found: {level_names}"


    if expect_levels is not None:
        assert len(level_names) >= expect_levels, f"Expected at least {expect_levels} level(s), found {len(level_names)}: {level_names}"

    shapes: list[tuple[int, ...]] = []
    for lvl in level_names:
        node = series0[str(lvl)]

        # Level can be a direct array: data.zarr/0/<lvl>
        if isinstance(node, zarr.Array):
            shapes.append(node.shape)
            continue

        # Or a group containing one or more arrays: data.zarr/0/<lvl>/<array>
        if isinstance(node, zarr.Group):
            array_keys = list(node.array_keys())
            assert array_keys, f"No arrays found in level group {lvl}. keys={list(node.keys())}"
            arr = node[array_keys[0]]
            shapes.append(arr.shape)
            continue

        raise AssertionError(f"Unexpected type at level {lvl}: {type(node)}")

    assert shapes, "No shapes collected from pyramid levels"
    y0, x0 = shapes[0][-2], shapes[0][-1]
    assert y0 > 0 and x0 > 0, "Each array must have non-zero shape"

    # pyramid monotonicity: dims should not increase
    for prev, nxt in zip(shapes, shapes[1:]):
        assert len(prev) == len(nxt), f"Rank changed: {prev} -> {nxt}"
        assert all(n <= p for p, n in zip(prev, nxt)), f"Not a pyramid: {prev} -> {nxt}"

    return shapes
