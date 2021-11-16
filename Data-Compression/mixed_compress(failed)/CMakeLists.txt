cmake_minimum_required(VERSION 3.10)
project(Data_Compression)

set(CMAKE_CXX_STANDARD 14)

add_executable(compress_engine_1 mix_compress_engine_1.cpp ../d_compress/src/frame_io.cpp ../d_compress/src/coding_1.cpp ../d_compress/include/coding_1.h
        lz_compress/include/lz_div.h lz_compress/src/lz_div.cpp
        )

#add_executable(decompress_engine_1 d_compress/decompress_engine_1.cpp d_compress/src/frame_io.cpp d_compress/src/coding_1.cpp)