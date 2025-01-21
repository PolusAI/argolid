#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include <filesystem>
#include <tuple>

#include "tensorstore/tensorstore.h"
#include "tensorstore/spec.h"

namespace fs = std::filesystem;
namespace argolid {
static constexpr int CHUNK_SIZE = 1024;

// custom hashing for using std::tuple as key
struct TupleHash {
    template <typename... Args>
    std::size_t operator()(const std::tuple<Args...>& t) const {
        return std::apply([](const auto&... args) {
            return (std::hash<std::decay_t<decltype(args)>>{}(args) ^ ...);
        }, t);
    }
};

// using image_data = std::variant<std::vector<std::uint8_t>,
//                                 std::vector<std::uint16_t>, 
//                                 std::vector<std::uint32_t>, 
//                                 std::vector<std::uint64_t>, 
//                                 std::vector<std::int8_t>, 
//                                 std::vector<std::int16_t>,
//                                 std::vector<std::int32_t>,
//                                 std::vector<std::int64_t>,
//                                 std::vector<float>,
//                                 std::vector<double>>;

using image_data = std::variant<tensorstore::Array<std::uint8_t, 2>,
                                tensorstore::Array<std::uint16_t, 2>, 
                                tensorstore::Array<std::uint32_t, 2>, 
                                tensorstore::Array<std::uint64_t, 2>, 
                                tensorstore::Array<std::int8_t, 2>, 
                                tensorstore::Array<std::int16_t, 2>,
                                tensorstore::Array<std::int32_t, 2>,
                                tensorstore::Array<std::int64_t, 2>,
                                tensorstore::Array<float, 2>,
                                tensorstore::Array<double, 2>,

                                tensorstore::Array<std::uint8_t, 3>,
                                tensorstore::Array<std::uint16_t, 3>, 
                                tensorstore::Array<std::uint32_t, 3>, 
                                tensorstore::Array<std::uint64_t, 3>, 
                                tensorstore::Array<std::int8_t, 3>, 
                                tensorstore::Array<std::int16_t, 3>,
                                tensorstore::Array<std::int32_t, 3>,
                                tensorstore::Array<std::int64_t, 3>,
                                tensorstore::Array<float, 3>,
                                tensorstore::Array<double, 3>,

                                tensorstore::Array<std::uint8_t, 4>,
                                tensorstore::Array<std::uint16_t, 4>, 
                                tensorstore::Array<std::uint32_t, 4>, 
                                tensorstore::Array<std::uint64_t, 4>, 
                                tensorstore::Array<std::int8_t, 4>, 
                                tensorstore::Array<std::int16_t, 4>,
                                tensorstore::Array<std::int32_t, 4>,
                                tensorstore::Array<std::int64_t, 4>,
                                tensorstore::Array<float, 4>,
                                tensorstore::Array<double, 4>,

                                tensorstore::Array<std::uint8_t, 5>,
                                tensorstore::Array<std::uint16_t, 5>, 
                                tensorstore::Array<std::uint32_t, 5>, 
                                tensorstore::Array<std::uint64_t, 5>, 
                                tensorstore::Array<std::int8_t, 5>, 
                                tensorstore::Array<std::int16_t, 5>,
                                tensorstore::Array<std::int32_t, 5>,
                                tensorstore::Array<std::int64_t, 5>,
                                tensorstore::Array<float, 5>,
                                tensorstore::Array<double, 5>>;
class Seq
{
    private:
        long start_index_, stop_index_, step_;
    public:
        inline Seq(const long start, const long  stop, const long  step=1):start_index_(start), stop_index_(stop), step_(step){} 
        inline long Start()  const  {return start_index_;}
        inline long Stop()  const {return stop_index_;}
        inline long Step()  const {return step_;}
};

class PyramidCompositor {
public:

    PyramidCompositor(const std::string& well_pyramid_loc, const std::string& out_dir, const std::string& pyramid_file_name);

    void reset_composition();

    void create_xml();
    void create_zattr_file();
    void create_zgroup_file();
    void create_auxiliary_files();

    template <typename T>
    void _write_zarr_chunk(int level, int channel, int y_index, int x_index);

    void write_zarr_chunk(int level, int channel, int y_index, int x_index);

    void setComposition(const std::unordered_map<std::tuple<int, int, int>, std::string, TupleHash>& comp_map);
private:
    
    template <typename T>
    void WriteImageData(
        tensorstore::TensorStore<void, -1, tensorstore::ReadWriteMode::dynamic>& source,
        std::vector<T>& image,
        const Seq& rows,
        const Seq& cols,
        const std::optional<Seq>& layers,
        const std::optional<Seq>& channels,
        const std::optional<Seq>& tsteps);

    auto ReadImageData(
        const std::string& fname,
        const Seq& rows, 
        const Seq& cols, 
        const Seq& layers = Seq(0,0), 
        const Seq& channels = Seq(0,0), 
        const Seq& tsteps = Seq(0,0));

    // std::tuple<std::shared_ptr<image_data>, std::vector<std::int64_t>> ReadImageData(
    //     const std::string& fname,  
    //     const Seq& rows, const Seq& cols, 
    //     const Seq& layers = Seq(0,0), 
    //     const Seq& channels = Seq(0,0), 
    //     const Seq& tsteps = Seq(0,0)
    // );

    image_data GetAssembledImageVector(int size);

    // members for pyramid composition 
    std::string _input_pyramids_loc;
    std::filesystem::path _output_pyramid_name;
    std::string _ome_metadata_file;
    std::unordered_map<int, std::vector<std::int64_t>> _plate_image_shapes;
    // std::unordered_map<int, std::tuple< ,std::tuple<int, int, int, int, int>> _zarr_arrays;
    //std::unordered_map<int, <std::vector<std::int64_t>>> _zarr_arrays; // tuple at 0 is the image and at 1 is the size

    std::unordered_map<int, tensorstore::TensorStore<void, -1, tensorstore::ReadWriteMode::dynamic>> _zarr_arrays;

    std::unordered_map<int, std::pair<int, int>> _unit_image_shapes;
    std::unordered_map<std::tuple<int, int, int>, std::string, TupleHash> _composition_map;

    std::set<std::tuple<int, int, int, int>> _tile_cache;
    int _pyramid_levels;
    int _num_channels;

    tensorstore::DataType _image_ts_dtype;
    std::string _image_dtype;
    std::uint16_t _image_dtype_code;

    std::optional<int>_z_index, _c_index, _t_index;
    int _x_index, _y_index;
};
} // ns argolid
