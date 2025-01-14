#include "pyramid_compositor.h"
#include "../utilities/utilities.h"

#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cmath>

#include "tensorstore/context.h"
#include "tensorstore/array.h"
#include "tensorstore/driver/zarr/dtype.h"
#include "tensorstore/index_space/dim_expression.h"
#include "tensorstore/kvstore/kvstore.h"
#include "tensorstore/open.h"

#include <nlohmann/json.hpp>

using ::tensorstore::internal_zarr::ChooseBaseDType;
using json = nlohmann::json;

namespace fs = std::filesystem;

namespace argolid {
PyramidCompositor::PyramidCompositor(const std::string& input_pyramids_loc, const std::string& out_dir, const std::string& output_pyramid_name): 
    _input_pyramids_loc(input_pyramids_loc),
    _output_pyramid_name(out_dir + "/" + output_pyramid_name),
    _ome_metadata_file(out_dir + "/" + output_pyramid_name + "/METADATA.ome.xml"),
    _pyramid_levels(-1),
    _num_channels(-1) {


    tensorstore::TensorStore<void, -1, tensorstore::ReadWriteMode::dynamic> source;

    TENSORSTORE_CHECK_OK_AND_ASSIGN(source, tensorstore::Open(
                GetZarrSpecToRead(input_pyramids_loc),
                tensorstore::OpenMode::open,
                tensorstore::ReadWriteMode::read).result());

    _image_dtype = source.dtype().name();
    _data_type_code = GetDataTypeCode(_image_dtype);

    // auto read_spec = GetZarrSpecToRead(input_pyramids_loc)

    // TENSORSTORE_CHECK_OK_AND_ASSIGN(source, tensorstore::Open(
    //             read_spec,
    //             tensorstore::OpenMode::open,
    //             tensorstore::ReadWriteMode::read).result());
    
    // auto image_shape = source.domain().shape();
    // const auto read_chunk_shape = source.chunk_layout().value().read_chunk_shape();

    // if (image_shape.size() == 5){
    //     _image_height = image_shape[3];
    //     _image_width = image_shape[4];
    //     _image_depth = image_shape[2];
    //     _num_channels = image_shape[1];
    //     _num_tsteps = image_shape[0];
    //     _t_index.emplace(0);
    //     _c_index.emplace(1);
    //     _z_index.emplace(2);
    // } else {
    //     assert(image_shape.size() >= 2);
    //     std::tie(_t_index, _c_index, _z_index) = ParseMultiscaleMetadata(axes_list, image_shape.size());
    //     _image_height = image_shape[image_shape.size()-2];
    //     _image_width = image_shape[image_shape.size()-1];
    //     if (_t_index.has_value()) {
    //         _num_tsteps = image_shape[_t_index.value()];
    //     } else {
    //         _num_tsteps = 1;
    //     }

    //     if (_c_index.has_value()) {
    //         _num_channels = image_shape[_c_index.value()];
    //     } else {
    //         _num_channels = 1;
    //     }

    //     if (_z_index.has_value()) {
    //         _image_depth = image_shape[_z_index.value()];
    //     } else {
    //         _image_depth = 1;
    //     }
    // }

    // _data_type = source.dtype().name();
    // _data_type_code = GetDataTypeCode(_data_type);

    // TENSORSTORE_CHECK_OK_AND_ASSIGN(___mb_cur_max, tensorstore::Open(
    //     GetZarrSpecToWrite(_filename, _image_shape, _chunk_shape, GetEncodedType(_dtype_code)),
    //     tensorstore::OpenMode::create |
    //     tensorstore::OpenMode::delete_existing,
    //     tensorstore::ReadWriteMode::write).result()
    // );
}

// Private Methods
void PyramidCompositor::create_xml() {
    CreateXML(
        this->_output_pyramid_name, 
        this->_ome_metadata_file,
        this->_plate_image_shapes[0],
        this->_image_dtype
    );
}

void PyramidCompositor::create_zattr_file() {
    WriteTSZattrFilePlateImage(
        this->_output_pyramid_name,
        this->_output_pyramid_name + "/data.zarr/0",
        this->_plate_image_shapes
    );
}

void PyramidCompositor::create_zgroup_file() {
    WriteVivZgroupFiles(this->_output_pyramid_name);
}

void PyramidCompositor::create_auxiliary_files() {
    create_xml();
    create_zattr_file();
    create_zgroup_file();
}

void PyramidCompositor::write_zarr_chunk(int level, int channel, int y_int, int x_int) {
    // Implementation of tile data writing logic
    // Placeholder logic

    // std::tuple<int, int> y_range = {
    //     y_int * CHUNK_SIZE,
    //     std::min((y_int+1) * CHUNK_SIZE, this->zarr_arrays_[level].size()[3])
    // };

    // std::tuple<int, int> x_range = {
    //     x_int * CHUNK_SIZE,
    //     min((x_int+1) * CHUNK_SIZE, this->zarr_arrays_[level].size()[4])
    // };

    // int assembled_width = std::get<1>(x_range) - std::get<0>(x_range);
    // int assembled_height = std::get<1>(y_range) - std::get<0>(y_range);

    // std::vector<>

    // Compute ranges in global coordinates
    int y_start = y_int * CHUNK_SIZE;
    int y_end = std::min((y_int + 1) * CHUNK_SIZE, std::get<1>(_zarr_arrays[level])[0]);
    int x_start = x_int * CHUNK_SIZE;
    int x_end = std::min((x_int + 1) * CHUNK_SIZE, std::get<1>(_zarr_arrays[level])[1]);

    int assembled_width = x_end - x_start;
    int assembled_height = y_end - y_start;
    
    image_data assembled_image = GetAssembledImageVector(assembled_height*assembled_width);

    // Determine required input images
    int unit_image_height = _unit_image_shapes[level].first;
    int unit_image_width = _unit_image_shapes[level].second;

    int row_start_pos = y_start;
    while (row_start_pos < y_end) {
        int row = row_start_pos / unit_image_height;
        int local_y_start = row_start_pos - y_start;
        int tile_y_start = row_start_pos - row * unit_image_height;
        int tile_y_dim = std::min((row + 1) * unit_image_height - row_start_pos, y_end - row_start_pos);
        int tile_y_end = tile_y_start + tile_y_dim;

        int col_start_pos = x_start;
        while (col_start_pos < x_end) {
            int col = col_start_pos / unit_image_width;
            int local_x_start = col_start_pos - x_start;
            int tile_x_start = col_start_pos - col * unit_image_width;
            int tile_x_dim = std::min((col + 1) * unit_image_width - col_start_pos, x_end - col_start_pos);
            int tile_x_end = tile_x_start + tile_x_dim;

            // Fetch input file path from composition map
            std::string input_file_name = _composition_map[{col, row, channel}];
            std::filesystem::path zarrArrayLoc = std::filesystem::path(input_file_name) / "data.zarr/0/" / std::to_string(level);

            auto read_data = std::get<0>(ReadImageData(zarrArrayLoc, Seq(tile_y_start, tile_y_end), Seq(tile_x_start, tile_x_end))).get();
            
            for (int i = local_y_start; i < local_y_start + tile_y_end - tile_y_start; ++i) {
                for (int j = local_x_start; i < local_x_start + tile_x_end - tile_x_start; ++j) {
                    assembled_image[i*(tile_x_end - tile_x_start) + j] = read_data[(i-local_y_start)*(tile_x_end - tile_x_start) + (j-local_x_start)];
                }
            }
            
            // // Open the Zarr array for reading with TensorStore
            // auto zarrSpec = tensorstore::Context::Default()
            //     | tensorstore::OpenMode::read
            //     | tensorstore::dtype_v<uint8_t>
            //     | tensorstore::Shape({1, 1, 1, assembledHeight, assembledWidth});

            // auto inputZarrArray = tensorstore::Open(zarrSpec).result();

            // TENSORSTORE_CHECK_OK_AND_ASSIGN()

            // if (!inputZarrArray.ok()) {
            //     std::cerr << "Failed to open Zarr array: " << inputZarrArray.status() << "\n";
            //     return;
            // }

            // // Read data from input Zarr file into assembledImage
            // tensorstore::Read(*inputZarrArray,
            //     tensorstore::Span<uint8_t>(assembledImage.data(), assembledImage.size())).result();

            col_start_pos += tile_x_end - tile_x_start;
        }
        row_start_pos += tile_y_end - tile_y_start;
    }

    
    WriteImageData(
        _input_pyramids_loc,
        std::get<0>(_zarr_arrays[level]).get(), 
        Seq(y_start,y_end), Seq(x_start, x_end), 
        Seq(0,0), Seq(channel, channel), Seq(0,0)
    );
}

void PyramidCompositor::setComposition(const std::unordered_map<std::tuple<int, int, int>, std::string>& comp_map) {
    _composition_map = comp_map;

    for (const auto& coord : comp_map) {
        std::string file_path = coord.second;
        std::filesystem::path attr_file_loc = std::filesystem::path(file_path) / "data.zarr/0/.zattrs";

        if (std::filesystem::exists(attr_file_loc)) {
            std::ifstream attr_file(attr_file_loc);
            json attrs;
            attr_file >> attrs;

            const auto& multiscale_metadata = attrs["multiscales"][0]["datasets"];
            _pyramid_levels = multiscale_metadata.size();
            for (const auto& dic : multiscale_metadata) {
                std::string res_key = dic["path"];
                std::filesystem::path zarr_array_loc = std::filesystem::path(file_path) / "data.zarr/0/" / res_key;

                auto read_spec = GetZarrSpecToRead(zarr_array_loc);

                tensorstore::TensorStore<void, -1, tensorstore::ReadWriteMode::dynamic> source;

                TENSORSTORE_CHECK_OK_AND_ASSIGN(source, tensorstore::Open(
                            read_spec,
                            tensorstore::OpenMode::open,
                            tensorstore::ReadWriteMode::read).result());
                
                auto image_shape = source.domain().shape();
                _image_dtype = source.dtype().name();
                _data_type_code = GetDataTypeCode(_image_dtype);

                _unit_image_shapes[std::stoi(res_key)] = {
                    image_shape[image_shape.size()-2],
                    image_shape[image_shape.size()-1]
                };
            }
            break;
        }
    }

    int num_rows = 0, num_cols = 0, num_channels = 0;
    for (const auto& coord : comp_map) {
        num_rows = std::max(num_rows, std::get<1>(coord.first));
        num_cols = std::max(num_cols, std::get<0>(coord.first));
        _num_channels = std::max(num_channels, std::get<2>(coord.first));
    }
    num_cols += 1;
    num_rows += 1;
    num_channels += 1;
    _num_channels = num_channels;

    for (const auto& [level, shape] : _unit_image_shapes) {
        std::string path = _output_pyramid_name + "/data.zarr/0/" + std::to_string(level);

        auto zarr_array_data  = ReadImageData(
            path, 
            Seq(0, num_rows * shape.first), 
            Seq(0, num_cols * shape.second), 
            Seq(0,0), 
            Seq(0, num_channels), 
            Seq(0,0)
        );
        
        _zarr_arrays[level] = zarr_array_data;
    }
}

void PyramidCompositor::reset_composition() {
    // Remove pyramid file and clear internal data structures
    fs::remove_all(_output_pyramid_name);
    _composition_map.clear();
    _plate_image_shapes.clear();
    _tile_cache.clear();
    _plate_image_shapes.clear();
    _zarr_arrays.clear();
}

image_data PyramidCompositor::GetAssembledImageVector(int size){
    switch (_image_dtype_code)
    {
    case (1):
        return std::vector<std::uint8_t>(size);
    case (2):
        return std::vector<std::uint16_t>(size);  
    case (4):
        return std::vector<std::uint32_t>(size);
    case (8):
        return std::vector<std::uint64_t>(size);
    case (16):
        return std::vector<std::int8_t>(size);
    case (32):
        return std::vector<std::int16_t>(size); 
    case (64):
        return std::vector<std::int32_t>(size);
    case (128):
        return std::vector<std::int64_t>(size);
    case (256):
        return std::vector<float>(size);
    case (512):
        return std::vector<double>(size);
    default:
        throw std::runtime_error("Invalid data type.");
    }
}

void PyramidCompositor::WriteImageData(
    const std::string& path,
    image_data& image,
    const Seq& rows,
    const Seq& cols,
    const std::optional<Seq>& layers,
    const std::optional<Seq>& channels,
    const std::optional<Seq>& tsteps) {

    std::vector<std::int64_t> shape;

    tensorstore::TensorStore<void, -1, tensorstore::ReadWriteMode::dynamic> source;

    TENSORSTORE_CHECK_OK_AND_ASSIGN(source, tensorstore::Open(
                GetZarrSpecToRead(path),
                tensorstore::OpenMode::open,
                tensorstore::ReadWriteMode::write).result());

    auto output_transform = tensorstore::IdentityTransform(source.domain());

    if (_t_index.has_value() && tsteps.has_value()) {
        output_transform = (std::move(output_transform) | tensorstore::Dims(_t_index.value()).ClosedInterval(tsteps.value().Start(), tsteps.value().Stop())).value();
        shape.emplace_back(tsteps.value().Stop() - tsteps.value().Start()+1);
    }

    if (_c_index.has_value() && channels.has_value()) {
        output_transform = (std::move(output_transform) | tensorstore::Dims(_c_index.value()).ClosedInterval(channels.value().Start(), channels.value().Stop())).value();
        shape.emplace_back(channels.value().Stop() - channels.value().Start()+1);
    }

    if (_z_index.has_value() && layers.has_value()) {
        output_transform = (std::move(output_transform) | tensorstore::Dims(_z_index.value()).ClosedInterval(layers.value().Start(), layers.value().Stop())).value();
        shape.emplace_back(layers.value().Stop() - layers.value().Start()+1);
    }

    output_transform = (std::move(output_transform) | tensorstore::Dims(_y_index).ClosedInterval(rows.Start(), rows.Stop()) |
                                                      tensorstore::Dims(_x_index).ClosedInterval(cols.Start(), cols.Stop())).value();

    shape.emplace_back(rows.Stop() - rows.Start()+1);
    shape.emplace_back(cols.Stop() - cols.Start()+1);

    auto data_array = tensorstore::Array(image.data(), shape, tensorstore::c_order);

    // Write data array to TensorStore
    auto write_result = tensorstore::Write(tensorstore::UnownedToShared(data_array), source | output_transform).result();

    if (!write_result.ok()) {
        std::cerr << "Error writing image: " << write_result.status() << std::endl;
    }
}

template <typename T>
std::shared_ptr<std::vector<T>> PyramidCompositor::GetImageDataTemplated(const Seq& rows, const Seq& cols, const Seq& layers, const Seq& channels, const Seq& tsteps){

    const auto data_height = rows.Stop() - rows.Start() + 1;
    const auto data_width = cols.Stop() - cols.Start() + 1;
    const auto data_depth = layers.Stop() - layers.Start() + 1;
    const auto data_num_channels = channels.Stop() - channels.Start() + 1;
    const auto data_tsteps = tsteps.Stop() - tsteps.Start() + 1;

    auto read_buffer = std::make_shared<std::vector<T>>(data_height*data_width*data_depth*data_num_channels*data_tsteps); 
    auto array = tensorstore::Array(read_buffer->data(), {data_tsteps, data_num_channels, data_depth, data_height, data_width}, tensorstore::c_order);
    tensorstore::intTransform<> read_transform = tensorstore::IdentityTransform(source.domain());

    int x_int=1, y_int=0;
    if (_t_index.has_value()){
        read_transform = (std::move(read_transform) | tensorstore::Dims(_t_index.value()).ClosedInterval(tsteps.Start(), tsteps.Stop())).value();
        x_int++;
        y_int++;
    }
    if (_c_index.has_value()){
        read_transform = (std::move(read_transform) | tensorstore::Dims(_c_index.value()).ClosedInterval(channels.Start(), channels.Stop())).value();
        x_int++;
        y_int++;
    }
    if (_z_index.has_value()){
        read_transform = (std::move(read_transform) | tensorstore::Dims(_z_index.value()).ClosedInterval(layers.Start(), layers.Stop())).value();
        x_int++;
        y_int++;
    }
    read_transform = (std::move(read_transform) | tensorstore::Dims(y_int).ClosedInterval(rows.Start(), rows.Stop()) |
                                                tensorstore::Dims(x_int).ClosedInterval(cols.Start(), cols.Stop())).value(); 


    tensorstore::Read(source | read_transform, tensorstore::UnownedToShared(array)).value();

    return read_buffer;
}

std::tuple<std::shared_ptr<image_data>, std::vector<int>> PyramidCompositor::ReadImageData(
    const std::string& fname, 
    const Seq& rows, const Seq& cols, 
    const Seq& layers = Seq(0,0), 
    const Seq& channels = Seq(0,0), 
    const Seq& tsteps = Seq(0,0)) {
    
    auto read_spec = GetZarrSpecToRead(fname);

    tensorstore::TensorStore<void, -1, tensorstore::ReadWriteMode::dynamic> source;

    TENSORSTORE_CHECK_OK_AND_ASSIGN(source, tensorstore::Open(
                read_spec,
                tensorstore::OpenMode::open,
                tensorstore::ReadWriteMode::read).result());
    
    auto image_shape = source.domain().shape();
    const auto read_chunk_shape = source.chunk_layout().value().read_chunk_shape();
    
    // if (image_shape.size() == 5){
    //     _image_height = image_shape[3];
    //     _image_width = image_shape[4];
    //     _image_depth = image_shape[2];    
    //     _num_channels = image_shape[1];
    //     _num_tsteps = image_shape[0];
    //     _t_index.emplace(0);
    //     _c_index.emplace(1);
    //     _z_index.emplace(2);
    // } else {
    //     assert(image_shape.size() >= 2);
    //     std::tie(_t_index, _c_index, _z_index) = ParseMultiscaleMetadata(axes_list, image_shape.size());
    //     _image_height = image_shape[image_shape.size()-2];
    //     _image_width = image_shape[image_shape.size()-1];
    //     if (_t_index.has_value()) {
    //         _num_tsteps = image_shape[_t_index.value()];
    //     } else {
    //         _num_tsteps = 1;
    //     }

    //     if (_c_index.has_value()) {
    //         _num_channels = image_shape[_c_index.value()];
    //     } else {
    //         _num_channels = 1;
    //     }

    //     if (_z_index.has_value()) {
    //         _image_depth = image_shape[_z_index.value()];
    //     } else {
    //         _image_depth = 1;
    //     }
    // }
    
    _image_dtype = source.dtype().name();
    _data_type_code = GetDataTypeCode(_image_dtype);
    
    switch (_data_type_code)
    {
    case (1):
        return std::make_tuple(
            std::make_shared<image_data>(std::move(*(GetImageDataTemplated<std::uint8_t>(rows, cols, layers, channels, tsteps)))),
            std::make_tuple(image_shape[image_shape.size()-2], image_shape[image_shape.size()-1]));
    case (2):
        return std::make_tuple(
            std::make_shared<image_data>(std::move(*(GetImageDataTemplated<std::uint16_t>(rows, cols, layers, channels, tsteps)))),
            std::make_tuple(image_shape[image_shape.size()-2], image_shape[image_shape.size()-1]));   
    case (4):
        return std::make_tuple(
            std::make_shared<image_data>(std::move(*(GetImageDataTemplated<std::uint32_t>(rows, cols, layers, channels, tsteps)))),
            std::make_tuple(image_shape[image_shape.size()-2], image_shape[image_shape.size()-1]));
    case (8):
        return std::make_tuple( 
            std::make_shared<image_data>(std::move(*(GetImageDataTemplated<std::uint64_t>(rows, cols, layers, channels, tsteps)))),
            std::make_tuple(image_shape[image_shape.size()-2], image_shape[image_shape.size()-1]));
    case (16):
        return std::make_tuple(
            std::make_shared<image_data>(std::move(*(GetImageDataTemplated<std::int8_t>(rows, cols, layers, channels, tsteps)))),
            std::make_tuple(image_shape[image_shape.size()-2], image_shape[image_shape.size()-1]));
    case (32):
        return std::make_tuple(
            std::make_shared<image_data>(std::move(*(GetImageDataTemplated<std::int16_t>(rows, cols, layers, channels, tsteps)))),
            std::make_tuple(image_shape[image_shape.size()-2], image_shape[image_shape.size()-1]));   
    case (64):
        return std::make_tuple(
            std::make_shared<image_data>(std::move(*(GetImageDataTemplated<std::int32_t>(rows, cols, layers, channels, tsteps)))),
            std::make_tuple(image_shape[image_shape.size()-2], image_shape[image_shape.size()-1]));
    case (128):
        return std::make_tuple(
            std::make_shared<image_data>(std::move(*(GetImageDataTemplated<std::int64_t>(rows, cols, layers, channels, tsteps)))),
            std::make_tuple(image_shape[image_shape.size()-2], image_shape[image_shape.size()-1]));
    case (256):
        return std::make_tuple(
            std::make_shared<image_data>(std::move(*(GetImageDataTemplated<float>(rows, cols, layers, channels, tsteps)))),
            std::make_tuple(image_shape[image_shape.size()-2], image_shape[image_shape.size()-1]));
    case (512):
        return std::make_tuple(
            std::make_shared<image_data>(std::move(*(GetImageDataTemplated<double>(rows, cols, layers, channels, tsteps)))),
            std::make_tuple(image_shape[image_shape.size()-2], image_shape[image_shape.size()-1]));
    default:
        break;
    }
} 
} // ns argolid 