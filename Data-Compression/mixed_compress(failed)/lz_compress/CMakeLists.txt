cmake_minimum_required(VERSION 3.10)
project(Data_Compression)

set(CMAKE_CXX_STANDARD 14)

add_executable(lz_div compress_engine_1.cpp src/lz_div.cpp)
add_executable(mm src/main.cpp)

#add_executable(decompress_engine_1 d_compress/decompress_engine_1.cpp d_compress/src/frame_io.cpp d_compress/src/coding_1.cpp)