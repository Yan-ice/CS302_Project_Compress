//
// Created by 颜嘉钦 on 11/2/21.
//

#include <vector>
#ifndef SECOND_COMPRESS_LZ_DIV_H
#define SECOND_COMPRESS_LZ_DIV_H
#pragma once
using namespace std;



void lz_div(int division,const unsigned char* data, unsigned int position,const int read_size, vector<unsigned char> &out);

void lz_read(const unsigned char* data, unsigned int position,const int read_size);

void lz_reset();

void lz_decompress(int division,const vector<unsigned char> &data,vector<unsigned char> &out);

#endif //SECOND_COMPRESS_LZ_DIV_H
