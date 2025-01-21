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
    _output_pyramid_name(std::filesystem::path(out_dir) / output_pyramid_name),
    _ome_metadata_file(out_dir + "/" + output_pyramid_name + "/METADATA.ome.xml"),
    _pyramid_levels(-1),
    _num_channels(-1) {

    std::cerr << "Constructor" << std::endl;
    // tensorstore::TensorStore<void, -1, tensorstore::ReadWriteMode::dynamic> source;

    // TENSORSTORE_CHECK_OK_AND_ASSIGN(source, tensorstore::Open(
    //             GetZarrSpecToRead(input_pyramids_loc),
    //             tensorstore::OpenMode::open,
    //             tensorstore::ReadWriteMode::read).result());

    // tensorstore::DataType _image_ts_dtype = source.dtype();
    // _image_dtype = source.dtype().name();
    // _image_dtype_code = GetDataTypeCode(_image_dtype);

    // auto read_spec = GetZarrSpecToRead(input_pyramids_loc);

    // TENSORSTORE_CHECK_OK_AND_ASSIGN(source, tensorstore::Open(
    //             read_spec,
    //             tensorstore::OpenMode::open,
    //             tensorstore::ReadWriteMode::read).result());
    
    // auto image_shape = source.domain().shape();
    // const auto read_chunk_shape = source.chunk_layout().value().read_chunk_shape();

    // if (image_shape.size() == 5){
    //     _t_index.emplace(0);
    //     _c_index.emplace(1);
    //     _z_index.emplace(2);
    // } else {
    //     assert(image_shape.size() >= 2);
    //     std::tie(_t_index, _c_index, _z_index) = ParseMultiscaleMetadata("XYCTZ", image_shape.size());
    // }

    // _data_type = source.dtype().name();
    // _data_type_code = GetDataTypeCode(_data_type);

    // TENSORSTORE_CHECK_OK_AND_ASSIGN(___mb_cur_max, tensorstore::Open(
    //     GetZarrSpecToWrite(_filename, _image_shape, _chunk_shape, GetEncodedType(_dtype_code)),
    //     tensorstore::OpenMode::create |
    //     tensorstore::OpenMode::delete_existing,
    //     tensorstore::ReadWriteMode::write).result()
    // );

    std::cerr << "End Constructor" << std::endl;
}

// Private Methods
void PyramidCompositor::create_xml() {
    std::cerr << "XML" << std::endl;
    CreateXML(
        this->_output_pyramid_name, 
        this->_ome_metadata_file,
        this->_plate_image_shapes[0],
        this->_image_dtype
    );
    std::cerr << "End XML" << std::endl;
}

void PyramidCompositor::create_zattr_file() {
    std::cerr << "zattr" << std::endl;
    WriteTSZattrFilePlateImage(
        this->_output_pyramid_name,
        this->_output_pyramid_name.u8string() + "/data.zarr/0",
        this->_plate_image_shapes
    );

    std::cerr << "End zattr" << std::endl;
}

void PyramidCompositor::create_zgroup_file() {
    std::cerr << "zgroup" << std::endl;
    WriteVivZgroupFiles(this->_output_pyramid_name);
    std::cerr << "End zgroup" << std::endl;
}

void PyramidCompositor::create_auxiliary_files() {
    std::cerr << "aux" << std::endl;
    create_xml();
    create_zattr_file();
    create_zgroup_file();
    std::cerr << "End aux" << std::endl;
}

void PyramidCompositor::write_zarr_chunk(int level, int channel, int y_int, int x_int) {
    std::cerr << "zarr wrapper" << std::endl;

    // get datatype by opening
    std::string input_file_name = _composition_map[{x_int, y_int, channel}];
    std::filesystem::path zarrArrayLoc = _output_pyramid_name / input_file_name / std::filesystem::path("data.zarr/0/") / std::to_string(level);

    auto read_spec = GetZarrSpecToRead(zarrArrayLoc.u8string());

    std::cerr << "Read path 121: " << zarrArrayLoc.u8string() << std::endl;

    tensorstore::TensorStore<void, -1, tensorstore::ReadWriteMode::dynamic> source;

    TENSORSTORE_CHECK_OK_AND_ASSIGN(source, tensorstore::Open(
                read_spec,
                tensorstore::OpenMode::open,
                tensorstore::ReadWriteMode::read).result());
    
    auto data_type = GetDataTypeCode(source.dtype().name());

    switch(data_type){
        case 1:
            _write_zarr_chunk<uint8_t>(level, channel, y_int, x_int);
            break;
        case 2:
            _write_zarr_chunk<uint16_t>(level, channel, y_int, x_int);
            break;
        case 4:
            _write_zarr_chunk<uint32_t>(level, channel, y_int, x_int);
            break;
        case 8:
            _write_zarr_chunk<uint64_t>(level, channel, y_int, x_int);
            break;
        case 16:
            _write_zarr_chunk<int8_t>(level, channel, y_int, x_int);
            break;
        case 32:
            _write_zarr_chunk<int16_t>(level, channel, y_int, x_int);
            break;
        case 64:
            _write_zarr_chunk<int32_t>(level, channel, y_int, x_int);
            break;
        case 128:
            _write_zarr_chunk<int64_t>(level, channel, y_int, x_int);
            break;
        case 256:
            _write_zarr_chunk<float>(level, channel, y_int, x_int);
            break;
        case 512:
            _write_zarr_chunk<double>(level, channel, y_int, x_int);
            break;
        default:
            break;
        }

    std::cerr << "End zarr wrapper" << std::endl;
}

template <typename T>
void PyramidCompositor::_write_zarr_chunk(int level, int channel, int y_int, int x_int) {

    // Compute ranges in global coordinates
    std::cerr << "write zarr" << std::endl;
    auto image_shape = _zarr_arrays[level].domain().shape();
    int y_start = y_int * CHUNK_SIZE;
    int y_end = std::min((y_int + 1) * CHUNK_SIZE, (int)image_shape[3]);
    int x_start = x_int * CHUNK_SIZE;
    int x_end = std::min((x_int + 1) * CHUNK_SIZE, (int)image_shape[4]);

    int assembled_width = x_end - x_start;
    int assembled_height = y_end - y_start;

    
    std::vector<T> assembled_image_vec(assembled_width*assembled_height);

    // Determine required input images
    int unit_image_height = _unit_image_shapes[level].first;
    int unit_image_width = _unit_image_shapes[level].second;

    int row_start_pos = y_start;

    std::cerr << "1" << std::endl;
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
            std::cerr << "2" << std::endl;
            // Fetch input file path from composition map
            std::string input_file_name = _composition_map[{col, row, channel}];
            std::filesystem::path zarrArrayLoc = std::filesystem::path(input_file_name) / "data.zarr/0/" / std::to_string(level);
            std::cerr << "3" << std::endl;
            // Read inage
            //auto read_data = ReadImageData(zarrArrayLoc, Seq(tile_y_start, tile_y_end), Seq(tile_x_start, tile_x_end));

            auto read_spec = GetZarrSpecToRead(zarrArrayLoc.u8string());
            std::cerr << "4" << std::endl;
            tensorstore::TensorStore<void, -1, tensorstore::ReadWriteMode::dynamic> source;

            TENSORSTORE_CHECK_OK_AND_ASSIGN(source, tensorstore::Open(
                        read_spec,
                        tensorstore::OpenMode::open,
                        tensorstore::ReadWriteMode::read).result());


            std::vector<T> read_buffer((tile_x_end-tile_x_start)*(tile_y_end-tile_y_start));
            auto array = tensorstore::Array(read_buffer.data(), {tile_y_end-tile_y_start, tile_x_end-tile_x_start}, tensorstore::c_order);

            tensorstore::IndexTransform<> read_transform = tensorstore::IdentityTransform(source.domain());
            std::cerr << "5" << std::endl;
            Seq rows = Seq(y_start, y_end);
            Seq cols = Seq(x_start, x_end); 
            Seq tsteps = Seq(0, 0);
            Seq channels = Seq(channel-1, channel); 
            Seq layers = Seq(0, 0);

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
            read_transform = (std::move(read_transform) | tensorstore::Dims(3).ClosedInterval(rows.Start(), rows.Stop()-1) |
                                                        tensorstore::Dims(4).ClosedInterval(cols.Start(), cols.Stop()-1)).value();

            std::cerr << "rows start: " << rows.Start() << ", rows stop: " << rows.Stop() << std::endl;
            std::cerr << "rows start: " << cols.Start() << ", rows stop: " << cols.Stop() << std::endl;
            std::cerr << "6" << std::endl;
            tensorstore::Read(source | read_transform, tensorstore::UnownedToShared(array)).value();
            std::cerr << "7" << std::endl;
            for (int i = local_y_start; i < local_y_start + tile_y_end - tile_y_start; ++i) {
                for (int j = local_x_start; j < local_x_start + tile_x_end - tile_x_start; ++j) {
                    assembled_image_vec[i * assembled_width + j] = read_buffer[(i - local_y_start) * (tile_x_end - tile_x_start) + (j - local_x_start)];
                }
            }
            std::cerr << "8" << std::endl;
            // // auto read_spec = GetZarrSpecToRead(zarrArrayLoc.u8string());

            // // tensorstore::TensorStore<void, -1, tensorstore::ReadWriteMode::dynamic> source;

            // // TENSORSTORE_CHECK_OK_AND_ASSIGN(source, tensorstore::Open(
            // //             read_spec,
            // //             tensorstore::OpenMode::open,
            // //             tensorstore::ReadWriteMode::read).result());
            
            // // auto image_shape = source.domain().shape();
            // // const auto read_chunk_shape = source.chunk_layout().value().read_chunk_shape();

            // // auto image_width = image_shape[4];
            // // auto image_height = image_shape[3];

            // // auto array = tensorstore::AllocateArray({
            // //     image_height,
            // //     image_width
            // //     }, tensorstore::c_order,
            // //     tensorstore::value_init, source.dtype());
            
            // // // initiate a read
            // // tensorstore::Read(source |
            // //     tensorstore::Dims(3).ClosedInterval(0, image_height - 1) |
            // //     tensorstore::Dims(4).ClosedInterval(0, image_width - 1),
            // //     array).value();

            col_start_pos += tile_x_end - tile_x_start;
        }
        row_start_pos += tile_y_end - tile_y_start;
    }
    std::cerr << "9" << std::endl;
    auto& write_source = _zarr_arrays[level];
    std::cerr << "9.1" << std::endl;
    WriteImageData(
        write_source,
        assembled_image_vec,  // Access the data from the std::shared_ptr
        Seq(y_start, y_end), 
        Seq(x_start, x_end), 
        Seq(0, 0), 
        Seq(channel, channel), 
        Seq(0, 0)
    );

    std::cerr << "10" << std::endl;
    //auto data = *(std::get<0>(_zarr_arrays[level]).get());
     // WriteImageData(
    //     _input_pyramids_loc,
    //     data, 
    //     Seq(y_start,y_end), Seq(x_start, x_end), 
    //     Seq(0,0), Seq(channel, channel), Seq(0,0)
    // );

    // Assuming _zarr_arrays[level] is of type std::tuple<std::shared_ptr<std::variant<...>>, std::vector<long long>>
    //auto& [variant_ptr, some_vector] = _zarr_arrays[level]; // Structured binding (C++17)

    // std::visit([&](const auto& array) {
    //     using T = std::decay_t<decltype(array)>;

    //     // Use external variables and call WriteImageData
    //     WriteImageData(
    //         _input_pyramids_loc,
    //         array,  // Access the data from the std::shared_ptr
    //         Seq(y_start, y_end), 
    //         Seq(x_start, x_end), 
    //         Seq(0, 0), 
    //         Seq(channel, channel), 
    //         Seq(0, 0)
    //     );
    //}, data); // Dereference the shared_ptr to get the variant

    std::cerr << "end write zarr" << std::endl;
}

void PyramidCompositor::setComposition(const std::unordered_map<std::tuple<int, int, int>, std::string, TupleHash>& comp_map) {
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
                _image_ts_dtype = source.dtype();
                _image_dtype = source.dtype().name();
                _image_dtype_code = GetDataTypeCode(_image_dtype);

                std::cerr << "image dims: ";
                for (auto& dim: image_shape) {
                    std::cerr << dim << ", ";
                }
                std::cerr << std::endl;

                _unit_image_shapes[std::stoi(res_key)] = {
                    image_shape[image_shape.size()-2],
                    image_shape[image_shape.size()-1]
                };
            }
            break;
        }
    }

    int num_rows = 0, num_cols = 0, _num_channels = 0;
    for (const auto& coord : comp_map) {
        num_rows = std::max(num_rows, std::get<1>(coord.first));
        num_cols = std::max(num_cols, std::get<0>(coord.first));
        _num_channels = std::max(_num_channels, std::get<2>(coord.first));
    }

    num_cols += 1;
    num_rows += 1;
    _num_channels += 1;

    std::cerr << "NUM CHANNELS: " << _num_channels << std::endl;

    _plate_image_shapes.clear();
    _zarr_arrays.clear();

    for (auto& [level, shape]: _unit_image_shapes) {
        
        _plate_image_shapes[level] = std::vector<std::int64_t> {
            1,
            _num_channels,
            1,
            num_rows * shape.first,
            num_cols * shape.second
        };
    }

    for (const auto& [level, shape] : _unit_image_shapes) {
        std::string path = _output_pyramid_name.u8string() + "/data.zarr/0/" + std::to_string(level);

        tensorstore::TensorStore<void, -1, tensorstore::ReadWriteMode::dynamic> source;

        TENSORSTORE_CHECK_OK_AND_ASSIGN(source, tensorstore::Open(
                    GetZarrSpecToWrite(
                        path,
                        _plate_image_shapes[level],
                        {1,1,1, CHUNK_SIZE, CHUNK_SIZE},
                        ChooseBaseDType(_image_ts_dtype).value().encoded_dtype
                    ),
                    tensorstore::OpenMode::create |
                    tensorstore::OpenMode::delete_existing,
                    tensorstore::ReadWriteMode::write).result());
        
        _zarr_arrays[level] = source;
    }
    create_auxiliary_files();

    // TODO: add in code to add size to _zarr_arrays and then use to fix seg fault
    //       when determining size at beinging of write_zarr
    // for (const auto& [level, shape] : _unit_image_shapes) {
    //     std::string path = _output_pyramid_name + "/data.zarr/0/" + std::to_string(level);

    //     auto zarr_array_data  = ReadImageData(
    //         path, 
    //         Seq(0, num_rows * shape.first), 
    //         Seq(0, num_cols * shape.second), 
    //         Seq(0,0), 
    //         Seq(0, num_channels), 
    //         Seq(0,0)
    //     );
        
    //     _zarr_arrays[level] = std::shared_ptr<image_data>(zarr_array_data);
    // }
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

template <typename T>
void PyramidCompositor::WriteImageData(
    tensorstore::TensorStore<void, -1, tensorstore::ReadWriteMode::dynamic>& source,
    std::vector<T>& image,
    const Seq& rows,
    const Seq& cols,
    const std::optional<Seq>& layers,
    const std::optional<Seq>& channels,
    const std::optional<Seq>& tsteps) {

    std::vector<std::int64_t> shape;

    auto result_array = tensorstore::Array(image.data(), {rows.Stop()-rows.Start(), cols.Stop()-cols.Start()}, tensorstore::c_order);

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

    output_transform = (std::move(output_transform) | tensorstore::Dims(1).ClosedInterval(channels.value().Start(), channels.value().Stop())).value();

    output_transform = (std::move(output_transform) | tensorstore::Dims(3).ClosedInterval(rows.Start(), rows.Stop()-1) |
                                                      tensorstore::Dims(4).ClosedInterval(cols.Start(), cols.Stop()-1)).value();

    shape.emplace_back(rows.Stop() - rows.Start()+1);
    shape.emplace_back(cols.Stop() - cols.Start()+1);

    // Write data array to TensorStore
    auto write_result = tensorstore::Write(tensorstore::UnownedToShared(result_array), source | output_transform).result();

    if (!write_result.ok()) {
        std::cerr << "Error writing image: " << write_result.status() << std::endl;
    }
}


auto PyramidCompositor::ReadImageData(
    const std::string& fname,
    const Seq& rows, 
    const Seq& cols, 
    const Seq& layers, 
    const Seq& channels, 
    const Seq& tsteps) {

    auto read_spec = GetZarrSpecToRead(fname);

    tensorstore::TensorStore<void, -1, tensorstore::ReadWriteMode::dynamic> source;

    TENSORSTORE_CHECK_OK_AND_ASSIGN(source, tensorstore::Open(
                read_spec,
                tensorstore::OpenMode::open,
                tensorstore::ReadWriteMode::read).result());

    auto image_shape = source.domain().shape();
    const auto read_chunk_shape = source.chunk_layout().value().read_chunk_shape();

    const auto data_height = rows.Stop() - rows.Start() + 1;
    const auto data_width = cols.Stop() - cols.Start() + 1;
    const auto data_depth = layers.Stop() - layers.Start() + 1;
    const auto data_num_channels = channels.Stop() - channels.Start() + 1;
    const auto data_tsteps = tsteps.Stop() - tsteps.Start() + 1;

    auto array = tensorstore::AllocateArray(
        {data_tsteps, data_num_channels, data_depth, data_height, data_width}, 
        tensorstore::c_order, 
        tensorstore::value_init, 
        source.dtype()
    );

    tensorstore::IndexTransform<> read_transform = tensorstore::IdentityTransform(source.domain());

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

    return array;
}

// std::tuple<std::shared_ptr<image_data>, std::vector<std::int64_t>> PyramidCompositor::ReadImageData(
//     const std::string& fname, 
//     const Seq& rows, const Seq& cols, 
//     const Seq& layers, 
//     const Seq& channels, 
//     const Seq& tsteps) {
    
//     auto read_spec = GetZarrSpecToRead(fname);

//     tensorstore::TensorStore<void, -1, tensorstore::ReadWriteMode::dynamic> source;

//     TENSORSTORE_CHECK_OK_AND_ASSIGN(source, tensorstore::Open(
//                 read_spec,
//                 tensorstore::OpenMode::open,
//                 tensorstore::ReadWriteMode::read).result());
    
//     auto image_shape = source.domain().shape();
//     const auto read_chunk_shape = source.chunk_layout().value().read_chunk_shape();
    
//     // if (image_shape.size() == 5){
//     //     _image_height = image_shape[3];
//     //     _image_width = image_shape[4];
//     //     _image_depth = image_shape[2];    
//     //     _num_channels = image_shape[1];
//     //     _num_tsteps = image_shape[0];
//     //     _t_index.emplace(0);
//     //     _c_index.emplace(1);
//     //     _z_index.emplace(2);
//     // } else {
//     //     assert(image_shape.size() >= 2);
//     //     std::tie(_t_index, _c_index, _z_index) = ParseMultiscaleMetadata(axes_list, image_shape.size());
//     //     _image_height = image_shape[image_shape.size()-2];
//     //     _image_width = image_shape[image_shape.size()-1];
//     //     if (_t_index.has_value()) {
//     //         _num_tsteps = image_shape[_t_index.value()];
//     //     } else {
//     //         _num_tsteps = 1;
//     //     }

//     //     if (_c_index.has_value()) {
//     //         _num_channels = image_shape[_c_index.value()];
//     //     } else {
//     //         _num_channels = 1;
//     //     }

//     //     if (_z_index.has_value()) {
//     //         _image_depth = image_shape[_z_index.value()];
//     //     } else {
//     //         _image_depth = 1;
//     //     }
//     // }
    
//     _image_dtype = source.dtype().name();
//     _image_dtype_code = GetDataTypeCode(_image_dtype);
    
//     switch (_data_type_code)
//     {
//     case (1):
//         return std::make_tuple(
//             std::make_shared<image_data>(std::move(*(GetImageDataTemplated<std::uint8_t>(source, rows, cols, layers, channels, tsteps)))),
//             std::vector<std::int64_t>{image_shape[image_shape.size()-2], image_shape[image_shape.size()-1]});
//     case (2):
//         return std::make_tuple(
//             std::make_shared<image_data>(std::move(*(GetImageDataTemplated<std::uint16_t>(source, rows, cols, layers, channels, tsteps)))),
//             std::vector<std::int64_t>{image_shape[image_shape.size()-2], image_shape[image_shape.size()-1]});  
//     case (4):
//         return std::make_tuple(
//             std::make_shared<image_data>(std::move(*(GetImageDataTemplated<std::uint32_t>(source, rows, cols, layers, channels, tsteps)))),
//             std::vector<std::int64_t>{image_shape[image_shape.size()-2], image_shape[image_shape.size()-1]});
//     case (8):
//         return std::make_tuple( 
//             std::make_shared<image_data>(std::move(*(GetImageDataTemplated<std::uint64_t>(source, rows, cols, layers, channels, tsteps)))),
//             std::vector<std::int64_t>{image_shape[image_shape.size()-2], image_shape[image_shape.size()-1]});
//     case (16):
//         return std::make_tuple(
//             std::make_shared<image_data>(std::move(*(GetImageDataTemplated<std::int8_t>(source, rows, cols, layers, channels, tsteps)))),
//             std::vector<std::int64_t>{image_shape[image_shape.size()-2], image_shape[image_shape.size()-1]});
//     case (32):
//         return std::make_tuple(
//             std::make_shared<image_data>(std::move(*(GetImageDataTemplated<std::int16_t>(source, rows, cols, layers, channels, tsteps)))),
//             std::vector<std::int64_t>{image_shape[image_shape.size()-2], image_shape[image_shape.size()-1]});  
//     case (64):
//         return std::make_tuple(
//             std::make_shared<image_data>(std::move(*(GetImageDataTemplated<std::int32_t>(source, rows, cols, layers, channels, tsteps)))),
//             std::vector<std::int64_t>{image_shape[image_shape.size()-2], image_shape[image_shape.size()-1]});
//     case (128):
//         return std::make_tuple(
//             std::make_shared<image_data>(std::move(*(GetImageDataTemplated<std::int64_t>(source, rows, cols, layers, channels, tsteps)))),
//             std::vector<std::int64_t>{image_shape[image_shape.size()-2], image_shape[image_shape.size()-1]});
//     case (256):
//         return std::make_tuple(
//             std::make_shared<image_data>(std::move(*(GetImageDataTemplated<float>(source, rows, cols, layers, channels, tsteps)))),
//             std::vector<std::int64_t>{image_shape[image_shape.size()-2], image_shape[image_shape.size()-1]});
//     case (512):
//         return std::make_tuple(
//             std::make_shared<image_data>(std::move(*(GetImageDataTemplated<double>(source, rows, cols, layers, channels, tsteps)))),
//             std::vector<std::int64_t>{image_shape[image_shape.size()-2], image_shape[image_shape.size()-1]});
//     default:
//         break;
//     }
// } 
} // ns argolid 