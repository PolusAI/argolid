import logging
from pathlib import Path

import bfio
import numpy as np
import pytest
import tensorstore as ts
import ome_types
import argolid

logger = logging.getLogger("bfio.test")



@pytest.fixture(scope="session")
def test_dir(tmp_path_factory: pytest.TempPathFactory) -> Path:
    """A per-session temp dir for generating all images and outputs."""
    return tmp_path_factory.mktemp("argolid_test_data")


@pytest.fixture(scope="session")
def input_tiles_dir(test_dir: Path) -> Path:
    """Create the input OME-TIFF tiles once per test session."""
    tiles_dir = test_dir / "data"
    tiles_dir.mkdir(parents=True, exist_ok=True)

    for ch in range(4):
        for r in range(3):
            for c in range(2):
                image_path = tiles_dir / f"test_image_r{r}_c{c}_ch{ch}.ome.tiff"

                # BioWriter expects X,Y ordering as named args, data written as [X, Y, Z, C, T]
                with bfio.BioWriter(
                    str(image_path),
                    backend="python",
                    X=1024,
                    Y=1024,
                    C=1,
                    Z=1,
                    T=1,
                ) as bw:
                    test_val = np.ones((1024, 1024), dtype=np.uint16)

                    # setting up test data for mean sampling
                    if r == 0 and c == 0:
                        test_val[0, 0] = 8
                        test_val[0, 1] = 9
                        test_val[1, 0] = 7
                        test_val[1, 1] = 14

                    # setting up test data for mode sampling
                    if r == 0 and c == 1 and ch == 1:
                        test_val[0, 0] = 8
                        test_val[0, 1] = 8
                        test_val[1, 0] = 9
                        test_val[1, 1] = 9

                    bw[0:1024, 0:1024, 0, 0, 0] = test_val

    return tiles_dir


@pytest.fixture(scope="session")
def single_channel_viv_zarr(input_tiles_dir: Path) -> Path:
    """
    Generate single-channel Viv pyramid once, return output *.zarr path.
    """
    input_dir = str(input_tiles_dir)
    file_pattern = "test_image_r{x:d+}_c{y:d+}_ch0.ome.tiff"
    image_name = "single_channel_image_viv"
    output_dir = str(input_tiles_dir)  # legacy wrote alongside input

    pyr_gen = argolid.PyramidGenerartor()
    pyr_gen.set_log_level(4)
    pyr_gen.generate_from_image_collection(
        input_dir, file_pattern, image_name, output_dir, 512, "Viv")

    return input_tiles_dir / f"{image_name}.zarr"


@pytest.fixture(scope="session")
def multiple_channel_viv_zarr(input_tiles_dir: Path) -> Path:
    """
    Generate multi-channel Viv pyramid once, return output *.zarr path.
    """
    input_dir = str(input_tiles_dir)
    file_pattern = "test_image_r{x:d+}_c{y:d+}_ch{c:d}.ome.tiff"
    image_name = "multiple_channel_image_viv"
    output_dir = str(input_tiles_dir)

    pyr_gen = argolid.PyramidGenerartor()
    pyr_gen.set_log_level(4)
    pyr_gen.generate_from_image_collection(
        input_dir, file_pattern, image_name, output_dir, 1024, "Viv")

    return input_tiles_dir / f"{image_name}.zarr"


# -----------------------
# Tests: single-channel
# -----------------------

def test_single_channel_omexml_metadata_exists(single_channel_viv_zarr: Path) -> None:
    ome_xml_path = single_channel_viv_zarr / "METADATA.ome.xml"
    assert ome_xml_path.is_file()


def test_single_channel_valid_omexml_metadata(single_channel_viv_zarr: Path) -> None:
    ome_xml_path = single_channel_viv_zarr / "METADATA.ome.xml"
    ome_metadata = ome_types.from_xml(str(ome_xml_path))

    assert len(ome_metadata.images) == 1
    assert len(ome_metadata.images[0].pixels.channels) == 1
    assert ome_metadata.images[0].pixels.type == ome_types.model.PixelType.UINT8
    assert ome_metadata.images[0].pixels.size_x == 3072
    assert ome_metadata.images[0].pixels.size_y == 2048
    assert ome_metadata.images[0].pixels.size_z == 1
    assert ome_metadata.images[0].pixels.size_c == 1
    assert ome_metadata.images[0].pixels.size_t == 1
    assert ome_metadata.images[0].pixels.dimension_order == ome_types.model.Pixels_DimensionOrder.XYZCT


def test_single_channel_num_pyramid_levels(single_channel_viv_zarr: Path) -> None:
    # expected 5 levels: 0..4 under data.zarr/0/<level>
    for i in range(5):
        assert (single_channel_viv_zarr / "data.zarr" / "0" / str(i)).is_dir()



def test_multiple_channel_valid_omexml_metadata(multiple_channel_viv_zarr: Path) -> None:
    ome_xml_path = multiple_channel_viv_zarr / "METADATA.ome.xml"
    ome_metadata = ome_types.from_xml(str(ome_xml_path))

    assert len(ome_metadata.images) == 1
    assert len(ome_metadata.images[0].pixels.channels) == 4
    assert ome_metadata.images[0].pixels.type == ome_types.model.PixelType.UINT8
    assert ome_metadata.images[0].pixels.size_x == 3072
    assert ome_metadata.images[0].pixels.size_y == 2048
    assert ome_metadata.images[0].pixels.size_z == 1
    assert ome_metadata.images[0].pixels.size_c == 4
    assert ome_metadata.images[0].pixels.size_t == 1
    assert ome_metadata.images[0].pixels.dimension_order == ome_types.model.Pixels_DimensionOrder.XYZCT


def test_multiple_channel_num_pyramid_levels(multiple_channel_viv_zarr: Path) -> None:
    # expected 4 levels: 0..3
    for i in range(4):
        assert (multiple_channel_viv_zarr / "data.zarr" / "0" / str(i)).is_dir()


def test_multiple_channel_base_layer_data(multiple_channel_viv_zarr: Path) -> None:
    # Base layer store: .../data.zarr/0/0
    base_path = multiple_channel_viv_zarr / "data.zarr" / "0" / "0"

    dataset_future = ts.open(
        {
            "driver": "zarr",
            "kvstore": {
                "driver": "file",
                "path": str(base_path),
            },
        }
    )
    dataset = dataset_future.result()

    assert dataset.shape == (1, 4, 1, 2048, 3072)

# test Viv compatible Zarr is produced
# test OmeXml metadata
    # num channels
# test correct number of pyramid layers are produced
# test single channel image
# test multi channel image
# test proper downsampling is done
    # mode_max, mode_min, mean
# test channel specific downsampling

# test Neuroglance compatible Zarr is produced

# test Precomputed Neuroglancer is produced.
